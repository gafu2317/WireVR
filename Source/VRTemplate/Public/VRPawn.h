#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Logging/LogMacros.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/MovementComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/Image.h"
#include "MotionControllerComponent.h"
#include "Components/AudioComponent.h"
//#include "Components/WidgetComponent.h"
#include "VRPawn.generated.h"

class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class VRTEMPLATE_API AVRPawn : public APawn
{
    GENERATED_BODY()

    /** MappingContext */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputMappingContext* DefaultMappingContext;

    /** Jump Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* JumpAction;

    /** Wire Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* ConnectWireAction_L;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* ConnectWireAction_R;

public:
    AVRPawn();

    // 入力が有効かどうか
    // 外部から管理可能
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool bEnableInpute = true;

    // スキン変更メソッド（ブループリントから呼び出し可能）
    UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Skin")
    void ChangeBodyMaterial(UMaterialInterface* NewMaterial);

    // 位置変更メソッド（ブループリントから呼び出し可能）
    UFUNCTION(BlueprintCallable, Category = "Manage")
    void SetPosition(FVector newPosition);

    // 名前同期時の処理
    //UFUNCTION()
    //void OnRep_PlayerName();

    // UI側の名前表示を更新する
    //void UpdateNameOnWidget();

    //virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    void Jump(const FInputActionValue& Value);

    virtual void NotifyControllerChanged() override;
    virtual void BeginPlay() override;
    virtual void Tick(float deltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // ワイヤー接続可否判定
    void CheckConnectable(int index, bool bForceUpdate);

    // ワイヤーを巻き取る
    FVector RetractWire(int index, float deltaTime);

    // コントローラーのワールド座標を取得
    FVector GetControllerLocation(int index) const;

    // コントローラーの正面方向を取得
    FVector GetControllerForward(int index) const;

    // ワイヤー機動の開始
    void AttachWire(int index);
    void AttachWire_L();
    void AttachWire_R();

    // ワイヤー機動の終了
    void DetachWire(int index);
    void DetachWire_L();
    void DetachWire_R();

    // サーバー側との同期
    UFUNCTION(Server, Reliable, WithValidation)
    void SyncWithServer_Tf(
        FVector rootPos,
        FRotator bodyRotation,
        FVector controllerPos_L,
        FVector controllerPos_R,
        FVector controllerForward_L,
        FVector controllerForward_R
    );
    UFUNCTION(Server, Reliable, WithValidation)
    void SyncWithServer_Switch(int index, bool isAttach, FVector anchorPos);


private:
    UPROPERTY(VisibleAnywhere)
    UCapsuleComponent* CapsuleComponent;
    UPROPERTY(VisibleAnywhere)
    UMovementComponent* MovementComponent;
    UPROPERTY(VisibleAnywhere)
    UCameraComponent* VRCamera;

    FVector CurrentVelocity = FVector::ZeroVector;
    float SlopeSin;
    bool bGrounded;
    APlayerController* PC;

    UPROPERTY(EditAnywhere, Category = "Move Settings")
    float JumpZSpeed = 500;

    UPROPERTY(EditAnywhere, Category = "Move Settings")
    float SlopeLimit = 45;

    UPROPERTY(EditAnywhere, Category = "Move Settings")
    float Gravity = 500;

    UPROPERTY(EditAnywhere, Category = "Move Settings")
    float StoppableSpeed = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Move Settings")
    float GroundFriction = 5.0f;

    UPROPERTY(EditAnywhere, Category = "Move Settings")
    float AirResistance = 0.5f;

    /* 左右の区別がある場合は左が[0]で右が[1]とする */

    // ワイヤーが接続されているか
    TArray<bool> bWireAttached;

    // 前フレームでワイヤーが接続可能だったか
    TArray<bool> bPrevConnectable;

    // アンカーの固定座標
    TArray < FVector > StaticAnchorLocation;

    // Spline に沿ってメッシュを描画する
    UPROPERTY(VisibleAnywhere, Category = "Wire")
    TArray< USplineMeshComponent*> SplineMeshComponent;
    UPROPERTY(EditAnywhere, Category = "Wire")
    TArray< UStaticMeshComponent*> StaticMeshComponent_Rep;

    // モーションコントローラー（左/右）
    UPROPERTY(VisibleAnywhere, Category = "Controller")
    TArray <UMotionControllerComponent*> MotionController;

    // ワイヤー銃（左/右）
    UPROPERTY(EditAnywhere, Category = "Controller")
    UStaticMeshComponent* WireGun_L;
    UPROPERTY(EditAnywhere, Category = "Controller")
    UStaticMeshComponent* WireGun_R;

    UPROPERTY(EditAnywhere, Category = "Wire Settings")
    float WireRange = 5000.0f; // ワイヤーの射程距離

    UPROPERTY(EditAnywhere, Category = "Wire Settings")
    float RetractSpeed = 100000.0f; // ワイヤー巻き取り速度

    UPROPERTY(EditAnywhere, Category = "Sound Effect")
    UAudioComponent* WireAttachAudio; // ワイヤー接続時のオーディオ

    UPROPERTY(EditAnywhere, Category = "Sound Effect")
    UAudioComponent* WindAudio; // 風のオーディオ

    UPROPERTY(EditAnywhere, Category = "Character")
    UStaticMeshComponent* CharacterBody; // キャラクターの体
    UPROPERTY(EditAnywhere, Category = "Character")
    UStaticMeshComponent* CharacterHand_L; // キャラクターの左手
    UPROPERTY(EditAnywhere, Category = "Character")
    UStaticMeshComponent* CharacterHand_R; // キャラクターの右手
    UPROPERTY(EditAnywhere, Category = "Character")
    USceneComponent* CharacterShoulder_L; // キャラクターの左肩
    UPROPERTY(EditAnywhere, Category = "Character")
    USceneComponent* CharacterShoulder_R; // キャラクターの右肩

    // ワイヤー接続時の振動
    UPROPERTY(EditAnywhere, Category = "ForceFeedbackEffect")
    UForceFeedbackEffect* FFE_AttachWire_L;
    UPROPERTY(EditAnywhere, Category = "ForceFeedbackEffect")
    UForceFeedbackEffect* FFE_AttachWire_R;
    // ワイヤー巻取り時の振動
    UPROPERTY(EditAnywhere, Category = "ForceFeedbackEffect")
    UForceFeedbackEffect* FFE_Retract_L;
    UPROPERTY(EditAnywhere, Category = "ForceFeedbackEffect")
    UForceFeedbackEffect* FFE_Retract_R;

    // 名前Widgetのコンポーネント
    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    //UWidgetComponent* NameWidget;

    // 表示するプレイヤー名（Replicated）
    //UPROPERTY(ReplicatedUsing = OnRep_PlayerName, VisibleAnywhere, BlueprintReadOnly, Category = "PlayerInfo")
    //FString PlayerName;
};