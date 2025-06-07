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
#include "Kismet/KismetMathLibrary.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AVRPawn::AVRPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    //SetReplicateMovement(false);

    CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
    RootComponent = CapsuleComponent;
    CapsuleComponent->InitCapsuleSize(150.f, 150.0f);
    CapsuleComponent->SetEnableGravity(false);
    CapsuleComponent->SetCollisionProfileName(TEXT("InvisibleWall"));

    MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MyMoveComp"));
    MovementComponent->SetUpdatedComponent(CapsuleComponent);

    //VRカメラ
    VRCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VRCamera"));
    VRCamera->SetupAttachment(RootComponent);
    VRCamera->bUsePawnControlRotation = false;
    VRCamera->AddLocalOffset(FVector::UpVector * 45);
    //VRCamera->bLockToHmd = false;

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
    WireGun_L->SetCollisionProfileName(TEXT("NoCollision"));
    WireGun_L->SetIsReplicated(true);

    // 右手コントローラー
    MotionController[1] = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("Controller_R"));
    MotionController[1]->SetupAttachment(RootComponent);
    MotionController[1]->SetTrackingSource(EControllerHand::Right);
    WireGun_R = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WireGun_R"));
    WireGun_R->SetupAttachment(MotionController[1]);
    WireGun_R->AddLocalOffset(FVector::UpVector * -5);
    WireGun_R->SetRelativeScale3D(FVector::OneVector * 0.1f);
    WireGun_R->SetCollisionProfileName(TEXT("NoCollision"));
    WireGun_R->SetIsReplicated(true);

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
    StaticMeshComponent_Rep.SetNum(2);
    StaticMeshComponent_Rep[0] = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent_Rep_L"));
    StaticMeshComponent_Rep[0]->CastShadow = false;
    StaticMeshComponent_Rep[0]->SetVisibility(false);
    StaticMeshComponent_Rep[0]->SetIsReplicated(true);
    StaticMeshComponent_Rep[0]->SetupAttachment(MotionController[0]);
    StaticMeshComponent_Rep[0]->SetOwnerNoSee(true);
    StaticMeshComponent_Rep[1] = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent_Rep_R"));
    StaticMeshComponent_Rep[1]->CastShadow = false;
    StaticMeshComponent_Rep[1]->SetVisibility(false);
    StaticMeshComponent_Rep[1]->SetIsReplicated(true);
    StaticMeshComponent_Rep[1]->SetupAttachment(MotionController[1]);
    StaticMeshComponent_Rep[1]->SetOwnerNoSee(true);

    // オーディオ関係
    WireAttachAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("WireAttachAudio"));
    WireAttachAudio->SetupAttachment(RootComponent);
    WindAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("WindAudio"));
    WindAudio->SetupAttachment(RootComponent);

    // キャラクター
    CharacterBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CharacterBody"));
    CharacterBody->SetupAttachment(RootComponent);
    CharacterBody->SetCollisionProfileName(TEXT("NoCollision"));
    CharacterBody->SetRelativeScale3D(FVector::OneVector * 1.8f);
    CharacterBody->AddLocalOffset(FVector::UpVector * -140);
    CharacterBody->SetOwnerNoSee(true);
    CharacterHand_L = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CharacterHand_L"));
    CharacterHand_R = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CharacterHand_R"));
    CharacterHand_L->SetupAttachment(MotionController[0]);
    CharacterHand_R->SetupAttachment(MotionController[1]);
    CharacterHand_L->SetCollisionProfileName(TEXT("NoCollision"));
    CharacterHand_R->SetCollisionProfileName(TEXT("NoCollision"));
    CharacterHand_L->SetRelativeScale3D(FVector::OneVector * 1.8f);
    CharacterHand_R->SetRelativeScale3D(FVector::OneVector * 1.8f);
    CharacterHand_L->SetOwnerNoSee(true);
    CharacterHand_R->SetOwnerNoSee(true);
    CharacterHand_L->SetIsReplicated(true);
    CharacterHand_R->SetIsReplicated(true);
    CharacterShoulder_L = CreateDefaultSubobject<USceneComponent>(TEXT("CharacterShoulder_L"));
    CharacterShoulder_R = CreateDefaultSubobject<USceneComponent>(TEXT("CharacterShoulder_R"));
    CharacterShoulder_L->AddLocalOffset(FVector::RightVector * -40);
    CharacterShoulder_R->AddLocalOffset(FVector::RightVector * 40);
    CharacterShoulder_L->SetupAttachment(CharacterBody);
    CharacterShoulder_R->SetupAttachment(CharacterBody);

    //その他配列の確保
    bWireAttached.SetNum(2);
    bPrevConnectable.SetNum(2);
    StaticAnchorLocation.SetNum(2);
}


void AVRPawn::BeginPlay()
{
    Super::BeginPlay();

    // 傾斜判定用sin値を事前計算
    SlopeSin = sinf(SlopeLimit / 180 * PI);

    // ワイヤー表示更新
    CheckConnectable(0, true);
    CheckConnectable(1, true);

    // プレイヤーコントローラーの取得
    PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

    WindAudio->SetVolumeMultiplier(0);

    UE_LOG(LogTemp, Log, TEXT("ver.6"));

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
    // 自身のものしか実行しない
    if (!IsLocallyControlled()) return;

    Super::Tick(deltaTime);

    // 重力演算
    CurrentVelocity += FVector::DownVector * Gravity * deltaTime;


    // 空気抵抗による減速処理
    CurrentVelocity *= (1 - AirResistance * deltaTime);


    // ワイヤー接続中は専用の演算
    FVector pullVelocity = FVector::ZeroVector;
    if (bWireAttached[0])
    {
        pullVelocity += RetractWire(0, deltaTime);
    }
    if (bWireAttached[1])
    {
        pullVelocity += RetractWire(1, deltaTime);
    }
    CurrentVelocity += pullVelocity * deltaTime;


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


    /* 位置ズレを防ぐため描画処理は移動処理の後に実行 */


    // 必要に応じた接続可否判定
    if (!bWireAttached[0])
        CheckConnectable(0, false);
    if (!bWireAttached[1])
        CheckConnectable(1, false);


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

    // 体の向きを調整
    if (VRCamera && CharacterBody)
    {
        // カメラの回転取得
        FRotator CameraRotation = VRCamera->GetComponentRotation();

        // Z軸（Yaw）だけ取り出し、他を固定
        FRotator NewRotation(0.0f, CameraRotation.Yaw, 0.0f);

        // 回転を適用
        CharacterBody->SetWorldRotation(NewRotation);
    }

    // 腕の向きを調整
    FVector StartLocation = CharacterHand_L->GetComponentLocation();
    FVector TargetLocation = CharacterShoulder_L->GetComponentLocation();
    FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetLocation);
    CharacterHand_L->SetWorldRotation(LookAtRotation);
    StartLocation = CharacterHand_R->GetComponentLocation();
    TargetLocation = CharacterShoulder_R->GetComponentLocation();
    LookAtRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetLocation);
    CharacterHand_R->SetWorldRotation(LookAtRotation);

    // カメラ回転
    //FRotator DeviceRotation;
    //FVector DeviceLocation;
    //UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(DeviceRotation, DeviceLocation);

    // サーバーへの同期
    SyncWithServer_Tf(
        RootComponent->GetComponentLocation(),
        CharacterBody->GetComponentRotation(),
        GetControllerLocation(0),
        GetControllerLocation(1),
        GetControllerForward(0),
        GetControllerForward(1)
    );
}


void AVRPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

        //ジャンプ
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AVRPawn::Jump);

        //ワイヤー接続
        EnhancedInputComponent->BindAction(ConnectWireAction_L, ETriggerEvent::Started, this, &AVRPawn::AttachWire_L);
        EnhancedInputComponent->BindAction(ConnectWireAction_R, ETriggerEvent::Started, this, &AVRPawn::AttachWire_R);

        //ワイヤー解除
        EnhancedInputComponent->BindAction(ConnectWireAction_L, ETriggerEvent::Completed, this, &AVRPawn::DetachWire_L);
        EnhancedInputComponent->BindAction(ConnectWireAction_R, ETriggerEvent::Completed, this, &AVRPawn::DetachWire_R);
    }
}


// ワイヤー接続の切り替え
void AVRPawn::AttachWire_L()
{
    AttachWire(0);
}
void AVRPawn::AttachWire_R()
{
    AttachWire(1);
}

void AVRPawn::DetachWire_L()
{
    DetachWire(0);
}
void AVRPawn::DetachWire_R()
{
    DetachWire(1);
}


// ワイヤー接続
void AVRPawn::AttachWire(int index)
{
    // 入力が有効でなければ終了
    if (!bEnableInpute) return;

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

        // マテリアルの切り替え
        SplineMeshComponent[index]->SetCustomPrimitiveDataFloat(0, 0);

        // 効果音の再生
        WireAttachAudio->Stop();
        WireAttachAudio->Play(0.0f);

        // サーバーへ同期
        SyncWithServer_Switch(index, true, StaticAnchorLocation[index]);
    }
}


// ワイヤー切断
void AVRPawn::DetachWire(int index)
{
    // 接続フラグを下ろす
    bWireAttached[index] = false;

    // マテリアルの切り替え
    CheckConnectable(index, true);

    // サーバーへ同期
    SyncWithServer_Switch(index, false, FVector::ZeroVector);
}



void AVRPawn::CheckConnectable(int index, bool bForceUpdate)
{
    // マテリアルに銃の位置を受け渡し
    FVector controllerPos = GetControllerLocation(index);
    SplineMeshComponent[index]->SetCustomPrimitiveDataVector4(1, controllerPos);

    // コントローラーの向きでレイを飛ばして接続可能判定
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


// ワイヤーを巻き取る
FVector AVRPawn::RetractWire(int index, float deltaTime)
{
    // 変数格納
    FVector toAnchor = StaticAnchorLocation[index] - GetControllerLocation(index);
    float distance = (float)toAnchor.Size();
    FVector direction = toAnchor.GetSafeNormal();

    // ワイヤー方向の速度を取得
    float dotProduct = FVector::DotProduct(CurrentVelocity, direction);

    // 外方向の速度を打ち消し
    if (dotProduct < 0)
        CurrentVelocity -= (direction * dotProduct);

    // 引き寄せ速度を返す
    return direction * RetractSpeed * deltaTime;
}


//コントローラー正面方向を取得
FVector AVRPawn::GetControllerForward(int index) const
{
    return MotionController[index] ? MotionController[index]->GetForwardVector() : FVector::ForwardVector;
}


//コントローラー位置を取得
FVector AVRPawn::GetControllerLocation(int index) const
{
    return 
        MotionController[index] ? 
        MotionController[index]->GetComponentLocation() + GetControllerForward(index) * 20 :
        GetActorLocation();
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


void AVRPawn::Jump(const FInputActionValue& Value)
{
    // 接地状態のみジャンプ可能
    if (bGrounded)
    {
        CurrentVelocity += FVector::UpVector * JumpZSpeed;
    }
}


bool AVRPawn::SyncWithServer_Tf_Validate(
    FVector rootPos,
    FRotator bodyRotation,
    FVector controllerPos_L,
    FVector controllerPos_R,
    FVector controllerForward_L,
    FVector controllerForward_R
)
{
    return true;
}
void AVRPawn::SyncWithServer_Tf_Implementation(
    FVector rootPos,
    FRotator bodyRotation,
    FVector controllerPos_L,
    FVector controllerPos_R,
    FVector controllerForward_L,
    FVector controllerForward_R
    ) 
{
    // クライアントからの呼び出しにのみ応答
    if (IsLocallyControlled()) return;

    // 各クライアントに伝達
    SyncMulticast_Tf_Implementation(
        rootPos,
        bodyRotation,
        controllerPos_L,
        controllerPos_R,
        controllerForward_L,
        controllerForward_R
    );
}
bool AVRPawn::SyncMulticast_Tf_Validate(
    FVector rootPos,
    FRotator bodyRotation,
    FVector controllerPos_L,
    FVector controllerPos_R,
    FVector controllerForward_L,
    FVector controllerForward_R
)
{
    return true;
}
void AVRPawn::SyncMulticast_Tf_Implementation(
    FVector rootPos,
    FRotator bodyRotation,
    FVector controllerPos_L,
    FVector controllerPos_R,
    FVector controllerForward_L,
    FVector controllerForward_R
    ) 
{
    // 自分の制御下なら実行しない
    if (IsLocallyControlled()) return;


    /* 位置や回転の算出・反映 */

    // 本体の位置
    RootComponent->SetWorldLocation(rootPos);

    // 体のメッシュの回転
    CharacterBody->SetWorldRotation(bodyRotation);

    // コントローラーの位置と回転
    MotionController[0]->SetWorldLocationAndRotation(
        controllerPos_L, 
        FRotationMatrix::MakeFromX(controllerForward_L).Rotator());
    MotionController[1]->SetWorldLocationAndRotation(
        controllerPos_R,
        FRotationMatrix::MakeFromX(controllerForward_R).Rotator());

    // 手の回転
    FVector TargetLocation = CharacterShoulder_L->GetComponentLocation();
    FRotator LookAtRotation = 
        UKismetMathLibrary::FindLookAtRotation(controllerPos_L - controllerForward_L * 20, TargetLocation);
    CharacterHand_L->SetWorldRotation(LookAtRotation);
    TargetLocation = CharacterShoulder_R->GetComponentLocation();
    LookAtRotation = 
        UKismetMathLibrary::FindLookAtRotation(controllerPos_R - controllerForward_R * 20, TargetLocation);
    CharacterHand_R->SetWorldRotation(LookAtRotation);

    // ワイヤー描画
    LookAtRotation = UKismetMathLibrary::FindLookAtRotation(controllerPos_L, StaticAnchorLocation[0]);
    StaticMeshComponent_Rep[0]->SetWorldRotation(LookAtRotation);
    float wireLength = (StaticAnchorLocation[0] - controllerPos_L).Size();
    StaticMeshComponent_Rep[0]->SetRelativeScale3D(FVector::OneVector + FVector::ForwardVector * (wireLength - 1));
    LookAtRotation = UKismetMathLibrary::FindLookAtRotation(controllerPos_R, StaticAnchorLocation[1]);
    StaticMeshComponent_Rep[1]->SetWorldRotation(LookAtRotation);
    wireLength = (StaticAnchorLocation[1] - controllerPos_R).Size();
    StaticMeshComponent_Rep[1]->SetRelativeScale3D(FVector::OneVector + FVector::ForwardVector * (wireLength - 1));
}


bool AVRPawn::SyncWithServer_Switch_Validate(int index, bool isAttach, FVector anchorPos)
{
    return true;
}
void AVRPawn::SyncWithServer_Switch_Implementation(int index, bool isAttach, FVector anchorPos)
{
    // クライアントからの呼び出しにのみ応答
    if (IsLocallyControlled()) return;

    // 各プレイヤーに伝達
    SyncMulticast_Switch_Implementation(index, isAttach, anchorPos);
}
bool AVRPawn::SyncMulticast_Switch_Validate(int index, bool isAttach, FVector anchorPos)
{
    return true;
}
void AVRPawn::SyncMulticast_Switch_Implementation(int index, bool isAttach, FVector anchorPos)
{
    // 自分の制御下なら実行しない
    if (IsLocallyControlled()) return;

    // ワイヤー表示の切り替え
    StaticMeshComponent_Rep[index]->SetVisibility(isAttach);

    // ワイヤー接続時は
    if (isAttach) 
    {
        // アンカー位置も格納
        StaticAnchorLocation[index] = anchorPos;
    }
}


void AVRPawn::ChangeBodyMaterial_Implementation(UMaterialInterface* NewMaterial)
{
    if (NewMaterial)
    {
        if (CharacterBody)
        {
            CharacterBody->SetMaterial(0, NewMaterial); // ElementIndex = 0 → 第一マテリアル
        }
        if (CharacterHand_L)
        {
            CharacterHand_L->SetMaterial(0, NewMaterial);
        }
        if (CharacterHand_R)
        {
            CharacterHand_R->SetMaterial(0, NewMaterial);
        }
    }
}


void AVRPawn::SetPosition(FVector newPosition)
{
    // ワイヤー切断
    DetachWire(0);
    DetachWire(1);

    // 慣性を消去
    CurrentVelocity = FVector::ZeroVector;

    // 位置の移動
    RootComponent->SetWorldLocation(newPosition);

    // ワイヤー表示更新
    CheckConnectable(0, true);
    CheckConnectable(1, true);
}