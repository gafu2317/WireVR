#include "VRPawn.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/SplineMeshComponent.h"
#include "Components/Image.h"
#include "InputActionValue.h"
#include "Components/AudioComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"

// Sets default values
AVRPawn::AVRPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
    RootComponent = CapsuleComponent;
    CapsuleComponent->InitCapsuleSize(42.f, 96.0f);
    CapsuleComponent->SetEnableGravity(false);
    CapsuleComponent->SetCollisionProfileName(TEXT("BlockAll"));

    MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MyMoveComp"));
    MovementComponent->SetUpdatedComponent(CapsuleComponent);

    //VRカメラ
    VRCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VRCamera"));
    VRCamera->SetupAttachment(RootComponent);
    VRCamera->bUsePawnControlRotation = false;
    VRCamera->AddLocalOffset(FVector::UpVector * 80);
    VRCamera->bLockToHmd = false;

    // コントローラー
    MotionController.SetNum(2);

    // 左手コントローラー
    MotionController[0] = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("Controller_L"));
    MotionController[0]->SetupAttachment(RootComponent);
    MotionController[0]->SetTrackingSource(EControllerHand::Left);
    WireGun_L = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WireGun_L"));
    WireGun_L->SetupAttachment(MotionController[0]);
    WireGun_L->AddLocalOffset(FVector::UpVector * -5);
    WireGun_L->SetRelativeScale3D(FVector::OneVector * 0.1f);

    // 右手コントローラー
    MotionController[1] = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("Controller_R"));
    MotionController[1]->SetupAttachment(RootComponent);
    MotionController[1]->SetTrackingSource(EControllerHand::Right);
    WireGun_R = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WireGun_R"));
    WireGun_R->SetupAttachment(MotionController[1]);
    WireGun_R->AddLocalOffset(FVector::UpVector * -5);
    WireGun_R->SetRelativeScale3D(FVector::OneVector * 0.1f);

    // ワイヤー用の Spline Mesh の作成
    SplineMeshComponent.SetNum(2);
    SplineMeshComponent[0] = CreateDefaultSubobject<USplineMeshComponent>(TEXT("SplineMeshComponent_L"));
    SplineMeshComponent[0]->SetStartScale(FVector2D::UnitVector * 0.005);
    SplineMeshComponent[0]->SetEndScale(FVector2D::UnitVector * 0.005);
    SplineMeshComponent[0]->CastShadow = false;
    SplineMeshComponent[1] = CreateDefaultSubobject<USplineMeshComponent>(TEXT("SplineMeshComponent_R"));
    SplineMeshComponent[1]->SetStartScale(FVector2D::UnitVector * 0.005);
    SplineMeshComponent[1]->SetEndScale(FVector2D::UnitVector * 0.005);
    SplineMeshComponent[1]->CastShadow = false;

    // オーディオ関係
    WireAttachAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("WireAttachAudio"));
    WireAttachAudio->SetupAttachment(RootComponent);
    WindAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("WindAudio"));
    WindAudio->SetupAttachment(RootComponent);

    //その他配列の確保
    bWireAttached.SetNum(2);
    bPrevConnectable.SetNum(2);
    CurrentWireLength.SetNum(2);
    AttachWireLength.SetNum(2);
    StaticAnchorLocation.SetNum(2);
}


void AVRPawn::BeginPlay()
{
    Super::BeginPlay();

    // 傾斜判定用sin値を事前計算
    SlopeSin = sinf(SlopeLimit / 180 * PI);

    // ワイヤー表示更新
    CheckConnectable(0, true);
    CheckConnectable(0, true);

    UE_LOG(LogTemp, Log, TEXT("ver.2.0"));

    if (MotionController[0]) {
        UE_LOG(LogTemp, Log, TEXT("MotionController[0] is found."));
    }
    else {
        UE_LOG(LogTemp, Log, TEXT("MotionController[0] is not found!"));
    }

    if (MotionController[1]) {
        UE_LOG(LogTemp, Log, TEXT("MotionController[1] is found."));
    }
    else {
        UE_LOG(LogTemp, Log, TEXT("MotionController[1] is not found!"));
    }

}


void AVRPawn::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    // 必要に応じた接続可否判定
    if (!bWireAttached[0])
        CheckConnectable(0, false);
    if (!bWireAttached[1])
        CheckConnectable(1, false);

    // 重力演算
    CurrentVelocity += FVector::DownVector * Gravity * deltaTime;


    // 空気抵抗による減速処理
    CurrentVelocity *= (1 - AirResistance * deltaTime);


    // ワイヤー接続中は専用の演算
    FVector pullVelocity = FVector::ZeroVector;
    if (bWireAttached[0] || bWireAttached[1])
    {
        pullVelocity = UpdateWireMovement(deltaTime);
        CurrentVelocity += pullVelocity * deltaTime;

        // ワイヤー描画
        if (bWireAttached[0])
            SplineMeshComponent[0]->SetStartAndEnd(
                GetControllerLocation(0), FVector::ZeroVector,
                StaticAnchorLocation[0], FVector::ZeroVector
            );
        if (bWireAttached[1])
            SplineMeshComponent[1]->SetStartAndEnd(
                GetControllerLocation(1), FVector::ZeroVector,
                StaticAnchorLocation[1], FVector::ZeroVector
            );
    }


    // 衝突付き移動
    bGrounded = false;
    FHitResult Hit;
    MovementComponent->SafeMoveUpdatedComponent(
        CurrentVelocity * deltaTime,
        CapsuleComponent->GetComponentQuat(),
        true,
        Hit);

    // 衝突があれば
    if (Hit.IsValidBlockingHit())
    {
        // 接地判定
        bGrounded = Hit.Normal.Z > SlopeSin;

        // 衝突後の速度
        FVector newVelcity = CurrentVelocity - Hit.Normal * FVector::DotProduct(CurrentVelocity, Hit.Normal);

        // 衝突対象が地面でワイヤーの巻取りがないなら停止
        if (bGrounded && pullVelocity.Size() < StoppableSpeed)
        {
            // 速さが規定値未満なら停止
            if (newVelcity.Size() < StoppableSpeed)
                CurrentVelocity = FVector::ZeroVector;
            // 規定値以上なら摩擦処理
            else
            {
                // 衝突面を滑るように移動
                MovementComponent->SlideAlongSurface(CurrentVelocity * deltaTime, 1.f - Hit.Time, Hit.Normal, Hit);
                CurrentVelocity = newVelcity * (1 - GroundFriction * deltaTime);
            }
        }

        // 壁面や天井は滑るように移動
        else
        {
            MovementComponent->SlideAlongSurface(CurrentVelocity * deltaTime, 1.f - Hit.Time, Hit.Normal, Hit);
            CurrentVelocity = newVelcity;
        }
    }


    // 風切り音の再生
    WindAudio->SetVolumeMultiplier(CurrentVelocity.Size() / 5000);



    // カメラ位置補正
    //VRCamera->SetRelativeLocation(FVector::UpVector * 80);


    // カメラ回転
    FRotator DeviceRotation;
    FVector DevicePosition;
    UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(DeviceRotation, DevicePosition);
    VRCamera->SetWorldRotation(DeviceRotation);
}


void AVRPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{

    UE_LOG(LogTemp, Log, TEXT("入力割り当て　開始"));

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

        //ジャンプ
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AVRPawn::Jump);

        //移動
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AVRPawn::Move);

        //ワイヤー接続・解除
        EnhancedInputComponent->BindAction(ToggleWireAction_L, ETriggerEvent::Started, this, &AVRPawn::ToggleWire_L);
        EnhancedInputComponent->BindAction(ToggleWireAction_R, ETriggerEvent::Started, this, &AVRPawn::ToggleWire_R);

        //ワイヤー巻き取り
        EnhancedInputComponent->BindAction(RetractWireAction_L, ETriggerEvent::Triggered, this, &AVRPawn::RetractWire_L);
        EnhancedInputComponent->BindAction(RetractWireAction_R, ETriggerEvent::Triggered, this, &AVRPawn::RetractWire_R);

        UE_LOG(LogTemp, Log, TEXT("入力割り当て　完了"));
    }
}


void AVRPawn::CheckConnectable(int index, bool bForceUpdate)
{
    // マテリアルに銃の位置を受け渡し
    FVector controllerPos = GetControllerLocation(index);
    SplineMeshComponent[index]->SetCustomPrimitiveDataVector4(1, controllerPos);

    // コントローラーの向きでレイを飛ばしてワイヤーを接続
    FVector Start = GetControllerLocation(index);
    FVector Forward = GetControllerForward(index);
    FVector End = Start + (Forward * WireRange);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        // 照準用Ray描画
        SplineMeshComponent[index]->SetStartAndEnd(
            controllerPos,
            FVector::ZeroVector,
            Hit.ImpactPoint,
            FVector::ZeroVector);


        // フラグの更新があれば
        if (!bPrevConnectable[index] || bForceUpdate)
        {
            // マテリアルの切り替え
            SplineMeshComponent[index]->SetCustomPrimitiveDataFloat(0, 1);

            // フラグ更新
            bPrevConnectable[index] = true;
        }
    }
    else
    {
        // 照準用Ray描画
        SplineMeshComponent[index]->SetStartAndEnd(
            controllerPos,
            FVector::ZeroVector,
            controllerPos + GetControllerForward(index) * WireRange,
            FVector::ZeroVector);


        // フラグの更新があれば
        if (bPrevConnectable[index] || bForceUpdate)
        {
            // マテリアルの切り替え
            SplineMeshComponent[index]->SetCustomPrimitiveDataFloat(0, 2);

            // フラグ更新
            bPrevConnectable[index] = false;
        }
    };
}


FVector AVRPawn::UpdateWireMovement(float deltaTime)
{
    // 変数格納
    TArray < FVector > controllerPos{ GetControllerLocation(0), GetControllerLocation(1) };
    TArray < FVector > toAnchor{ StaticAnchorLocation[0] - controllerPos[0], StaticAnchorLocation[1] - controllerPos[1] };
    TArray<float> distance{ (float)toAnchor[0].Size(), (float)toAnchor[1].Size() };
    TArray < FVector > direction{ toAnchor[0].GetSafeNormal(), toAnchor[1].GetSafeNormal() };

    // 引き寄せ速度を定義
    FVector pullVelocity = FVector::ZeroVector;

    // ワイヤー処理
    if (bWireAttached[0] && distance[0] > CurrentWireLength[0])
    {
        // ワイヤー方向の速度を取得
        float dotProduct = FVector::DotProduct(CurrentVelocity, direction[0]);

        // 外方向の速度を打ち消し
        if (dotProduct < 0)
            CurrentVelocity -= (direction[0] * dotProduct);

        // 引き寄せ速度の加算
        pullVelocity += direction[0] * (distance[0] - CurrentWireLength[0]) * 300;
    }
    // 右も同様に
    if (bWireAttached[1] && distance[1] > CurrentWireLength[1])
    {
        float dotProduct = FVector::DotProduct(CurrentVelocity, direction[1]);

        if (dotProduct < 0)
            CurrentVelocity -= (direction[1] * dotProduct);

        pullVelocity += direction[1] * (distance[1] - CurrentWireLength[1]) * 300;
    }

    // 引き寄せ速度を返す
    return pullVelocity;
}


//コントローラー位置を取得
FVector AVRPawn::GetControllerLocation(int index) const
{
    return MotionController[index] ? MotionController[index]->GetComponentLocation() : GetActorLocation();
}


//コントローラー正面方向を取得
FVector AVRPawn::GetControllerForward(int index) const
{
    return MotionController[index] ? MotionController[index]->GetForwardVector() : FVector::ForwardVector;
}


// ワイヤー接続の切り替え
void AVRPawn::ToggleWire(int index)
{

    UE_LOG(LogTemp, Log, TEXT("VRCamera Relative Location: %s"), *VRCamera->GetRelativeLocation().ToString());

    if (bWireAttached[index])
    {
        DetachWire(index);
    }
    else
    {
        AttachWire(index);
    }
}
void AVRPawn::ToggleWire_L()
{
    ToggleWire(0);
}
void AVRPawn::ToggleWire_R()
{
    ToggleWire(1);
}


// ワイヤー接続
void AVRPawn::AttachWire(int index)
{
    // コントローラーの向きでレイを飛ばしてワイヤーを接続
    FVector Start = GetControllerLocation(index);
    FVector Forward = GetControllerForward(index);
    FVector End = Start + (Forward * WireRange);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        // 接続フラグを立てる
        bWireAttached[index] = true;

        // 接続位置を記憶
        StaticAnchorLocation[index] = Hit.ImpactPoint;

        // 接続時にワイヤー長を現在の距離に設定
        CurrentWireLength[index] = FVector::Dist(GetControllerLocation(index), StaticAnchorLocation[index]);

        // マテリアルの切り替え
        SplineMeshComponent[index]->SetCustomPrimitiveDataFloat(0, 0);

        // 接続時の長さを記憶
        AttachWireLength[index] = CurrentWireLength[index];

        // 効果音の再生
        WireAttachAudio->Stop();
        WireAttachAudio->Play(0.0f);
    }
}


// ワイヤー切断
void AVRPawn::DetachWire(int index)
{
    // 接続フラグを下ろす
    bWireAttached[index] = false;

    // マテリアルの切り替え
    CheckConnectable(index, true);
}


// ワイヤーを巻き取る
void AVRPawn::RetractWire(int index)
{
    if (bWireAttached[index])
    {
        // アンカーまでの距離を算出
        float distance = (StaticAnchorLocation[index] - GetControllerLocation(index)).Size();

        // ワイヤーの長さを更新
        CurrentWireLength[index] =
            FMath::Clamp(distance - RetractSpeed * GetWorld()->GetDeltaSeconds(), 100, WireRange);

        // ワイヤー切断条件までワイヤーを巻き取っていたら切断
        if (CurrentWireLength[index] / AttachWireLength[index] < DetachRate)
        {
            UE_LOG(LogTemp, Log, TEXT("Detach"));
            DetachWire(index);
        }
    }
}
void AVRPawn::RetractWire_L()
{
    RetractWire(0);
}
void AVRPawn::RetractWire_R()
{
    RetractWire(1);
}


void AVRPawn::NotifyControllerChanged()
{
    Super::NotifyControllerChanged();

    // Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}


/* 開発用 */
void AVRPawn::Move(const FInputActionValue& Value)
{
    // 接地状態でなければ移動不可
    if (!bGrounded || CurrentVelocity.Z > JumpZSpeed - 10) return;

    // input is a Vector2D
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (MotionController[0] != nullptr)
    {
        // get forward vector
        const FVector ForwardDirection = GetControllerForward(0) - GetControllerForward(0).Z;

        // get right vector 
        const FVector RightDirection = MotionController[0]->GetRightVector() - MotionController[0]->GetRightVector().Z;

        // add movement 
        CurrentVelocity = (ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X) * MoveSpeed;
    }

    else if (Controller != nullptr)
    {
        // find out which way is forward
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // get forward vector
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

        // get right vector 
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // add movement 
        CurrentVelocity = (ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X) * MoveSpeed;
    }
}


void AVRPawn::Jump(const FInputActionValue& Value)
{
    // 接地状態のみジャンプ可能
    if (bGrounded)
    {
        CurrentVelocity += FVector::UpVector * JumpZSpeed;
    }
}

