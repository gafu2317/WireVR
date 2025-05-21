#include "WireCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/SplineMeshComponent.h"
#include "Components/Image.h"
#include "Components/AudioComponent.h"
#include "InputActionValue.h"

AWireCharacter::AWireCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Don't rotate when the controller rotates. Let that just affect the camera.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

    // Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
    // instead of recompiling to adjust them
    GetCharacterMovement()->JumpZVelocity = 700.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    // Create a camera boom (pulls in towards the player if there is a collision)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 450.0f; // The camera follows at this distance behind the character	
    CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
    CameraBoom->AddLocalOffset(FVector::UpVector * 120);

    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
    FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

    // Spline Mesh の作成
    SplineMeshComponent = CreateDefaultSubobject<USplineMeshComponent>(TEXT("SplineMeshComponent"));

    // アンカー用の SceneComponent を作成
    AnchorComponent = CreateDefaultSubobject<USceneComponent>(TEXT("AnchorComponent"));

    // オーディオ関係
    WireAttachAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("WireAttachAudio"));
    WireAttachAudio->SetupAttachment(RootComponent);

    // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
    // are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}


void AWireCharacter::SetCrosshairWidget(UImage* image)
{
    CrosshairImage = image;
    //UE_LOG(LogTemp, Log, TEXT("SetImage"));
}


void AWireCharacter::BeginPlay()
{
    Super::BeginPlay();
}


void AWireCharacter::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    // ワイヤー接続中は専用の演算
    if (bIsWireAttached)
    {
        UpdateWireMovement(deltaTime);
    }

    // ワイヤー未接続中で照準の画像が変数登録されているなら
    else if (CrosshairImage)
    {
        // 接続状態の変化があれば色を変更
        if (CheckConnectable() != bIsPrevConnectable)
        {
            bIsPrevConnectable = !bIsPrevConnectable;
            CrosshairImage->SetColorAndOpacity(bIsPrevConnectable ? FLinearColor::Green : FLinearColor::Red);
        }
    }
}


bool AWireCharacter::CheckConnectable()
{
    // カメラの向きでレイを飛ばしてチェック
    FVector Start = CameraBoom->GetComponentLocation();
    FVector Forward = FollowCamera->GetComponentRotation().Vector();
    FVector End = Start + (Forward * WireRange);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
}


void AWireCharacter::UpdateWireMovement(float deltaTime)
{
    FVector anchorPos = GetAnchorLocation();
    FVector playerPos = GetActorLocation();
    FVector ToAnchor = anchorPos - playerPos;
    float Distance = ToAnchor.Size();
    FVector Direction = ToAnchor.GetSafeNormal();

    //ワイヤー描画
    SplineMeshComponent->SetStartAndEnd(playerPos, FVector::ZeroVector, anchorPos, FVector::ZeroVector);

    //ワイヤーの範囲外なら
    if (Distance > CurrentWireLength)
    {
        // ワイヤー方向の速度を取得
        float DotProduct = FVector::DotProduct(GetCharacterMovement()->Velocity, Direction);

        // 逆方向の速度を打ち消し
        if (DotProduct < 0)
        {
            GetCharacterMovement()->Velocity -= (Direction * DotProduct);
        }

        // 地面にいて上に引っ張られているなら一時的に "Falling" モードに変更
        if (GetCharacterMovement()->IsMovingOnGround() && Direction.Z > 0.707)
        {
            GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        }

        // 引き寄せ処理（距離に応じて調整）
        float PullStrength = (Distance - CurrentWireLength) * 1000;
        FVector PullVelocity = Direction * PullStrength;
        GetCharacterMovement()->Velocity += PullVelocity * deltaTime;
    }
}


//アンカー位置を取得
FVector AWireCharacter::GetAnchorLocation() const
{
    if (AttachedActor && AttachedComponent)
    {
        return AnchorComponent->GetComponentLocation();
    }
    else
    {
        return StaticAnchorLocation;
    }
}


//ワイヤー接続の切り替え
void AWireCharacter::ToggleWire()
{
    if (bIsWireAttached)
    {
        DetachWire();
    }
    else
    {
        AttachWire();
    }
}


void AWireCharacter::AttachWire()
{
    // カメラの向きでレイを飛ばしてワイヤーを接続
    FVector Start = CameraBoom->GetComponentLocation();
    FVector Forward = FollowCamera->GetComponentRotation().Vector();
    FVector End = Start + (Forward * WireRange);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        // 接続フラグを立てる
        bIsWireAttached = true;


        // Movable かどうか判定
        if (Hit.GetActor()->IsRootComponentMovable())
        {
            // Movable → アンカーをアタッチ
            AnchorComponent->SetWorldLocation(Hit.ImpactPoint);
            AnchorComponent->AttachToComponent(Hit.GetComponent(), FAttachmentTransformRules::KeepWorldTransform);
            AttachedActor = Hit.GetActor();
            AttachedComponent = Hit.GetComponent();

            // "OnDestroyed" イベントを取得してコールバックを設定
            Hit.GetActor()->OnDestroyed.AddDynamic(this, &AWireCharacter::OnAttachedActorDestroyed);
        }
        else
        {
            // Static → 接続座標を保存
            StaticAnchorLocation = Hit.ImpactPoint;
        }

        // 接続時にワイヤー長を現在の距離に設定
        CurrentWireLength = FVector::Dist(GetActorLocation(), GetAnchorLocation());

        // ワイヤーを可視化
        SplineMeshComponent->SetVisibility(true);

        // 照準を透明に
        CrosshairImage->SetColorAndOpacity(FLinearColor::Transparent);

        // 効果音の再生
        WireAttachAudio->Stop();
        WireAttachAudio->Play(0.0f);
    }
}


void AWireCharacter::DetachWire()
{
    // 接続フラグを下ろす
    bIsWireAttached = false;

    // ワイヤーを不可視化
    SplineMeshComponent->SetVisibility(false);

    // イベントバインドを解除
    if (AttachedActor)
        AttachedActor->OnDestroyed.RemoveDynamic(this, &AWireCharacter::OnAttachedActorDestroyed);

    // アンカーの登録の解除
    AttachedActor = nullptr;
    AttachedComponent = nullptr;

    // 接続可否に応じて照準の色を変更
    CrosshairImage->SetColorAndOpacity(CheckConnectable() ? FLinearColor::Green : FLinearColor::Red);
}


// アタッチ先のアクターが削除されたら呼び出される
void AWireCharacter::OnAttachedActorDestroyed(AActor* DestroyedActor)
{
    // ワイヤー接続の解除
    DetachWire();
}


// Wキーでワイヤーを巻き取る
void AWireCharacter::RetractWire()
{
    if (bIsWireAttached)
    {
        float Distance = (GetAnchorLocation() - GetActorLocation()).Size();
        CurrentWireLength = FMath::Clamp(Distance - RetractSpeed * GetWorld()->GetDeltaSeconds(), 100.0f, WireMaxLength);
    }
}


// Sキーでワイヤーを伸ばす
void AWireCharacter::ExtendWire()
{
    if (bIsWireAttached)
    {
        float Distance = (GetAnchorLocation() - GetActorLocation()).Size();
        CurrentWireLength = FMath::Clamp(Distance + ExtendSpeed * GetWorld()->GetDeltaSeconds(), 100.0f, WireMaxLength);
    }
}


void AWireCharacter::NotifyControllerChanged()
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


void AWireCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // Set up action bindings
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

        //ジャンプ
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        //移動
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWireCharacter::Move);

        //視点移動
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWireCharacter::Look);

        //ワイヤー接続・解除
        EnhancedInputComponent->BindAction(ToggleWireAction, ETriggerEvent::Started, this, &AWireCharacter::ToggleWire);

        //ワイヤー巻き取り・伸ばし
        EnhancedInputComponent->BindAction(RetractWireAction, ETriggerEvent::Triggered, this, &AWireCharacter::RetractWire);
        EnhancedInputComponent->BindAction(ExtendWireAction, ETriggerEvent::Triggered, this, &AWireCharacter::ExtendWire);
    }
}


void AWireCharacter::Move(const FInputActionValue& Value)
{
    // input is a Vector2D
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // find out which way is forward
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // get forward vector
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

        // get right vector 
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // add movement 
        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}


void AWireCharacter::Look(const FInputActionValue& Value)
{
    // input is a Vector2D
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // add yaw and pitch input to controller
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}
