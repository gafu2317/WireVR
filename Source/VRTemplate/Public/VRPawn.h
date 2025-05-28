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

    /** Move Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* MoveAction;

    /** Wire Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* ConnectWireAction_L;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* ConnectWireAction_R;

public:
    AVRPawn();

protected:
    void Move(const FInputActionValue& Value); /* 開発用 */
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
    float MoveSpeed = 500;

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
    float AirResistance = 0.1f;

    /* 左右の区別がある場合は左が[0]で右が[1]とする */

    // ワイヤーが接続されているか
    TArray<bool> bWireAttached;

    // 前フレームでワイヤーが接続可能だったか
    TArray<bool> bPrevConnectable;

    // 現在のワイヤーの長さ
    TArray<float> CurrentWireLength;

    // アンカーの固定座標
    TArray < FVector > StaticAnchorLocation;

    // Spline に沿ってメッシュを描画する
    UPROPERTY(VisibleAnywhere, Category = "Wire")
    TArray< USplineMeshComponent*> SplineMeshComponent;

    // モーションコントローラー（左/右）
    UPROPERTY(VisibleAnywhere, Category = "Controller")
    TArray <UMotionControllerComponent*> MotionController;

    // ワイヤー銃（左/右）
    UPROPERTY(EditAnywhere, Category = "Controller")
    UStaticMeshComponent* WireGun_L;
    UPROPERTY(EditAnywhere, Category = "Controller")
    UStaticMeshComponent* WireGun_R;

    UPROPERTY(EditAnywhere, Category = "Wire Settings")
    float WireRange = 15000.0f; // ワイヤーの射程距離

    UPROPERTY(EditAnywhere, Category = "Wire Settings")
    float RetractSpeed = 180000.0f; // ワイヤー巻き取り速度

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
};