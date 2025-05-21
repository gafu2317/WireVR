#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "Logging/LogMacros.h"
#include "Components/SplineMeshComponent.h"
#include "Components/Image.h"
#include "Components/AudioComponent.h"
#include "WireCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class VRTEMPLATE_API AWireCharacter : public ACharacter
{
    GENERATED_BODY()

    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    /** MappingContext */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputMappingContext* DefaultMappingContext;

    /** Jump Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* JumpAction;

    /** Move Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* MoveAction;

    /** Look Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* LookAction;

    /** Wire Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* ToggleWireAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* RetractWireAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* ExtendWireAction;


public:
    AWireCharacter();

    // ウィジェットのインスタンスを格納
    UFUNCTION(BlueprintCallable)
    void SetCrosshairWidget(UImage* CrosshairImage);

protected:
    /** Called for movement input */
    void Move(const FInputActionValue& Value);

    /** Called for looking input */
    void Look(const FInputActionValue& Value);

    virtual void NotifyControllerChanged() override;
    virtual void BeginPlay() override;
    virtual void Tick(float deltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // ワイヤー接続の可否をチェック
    bool CheckConnectable();

    // ワイヤー機動の更新
    void UpdateWireMovement(float deltaTime);

    // アンカーのワールド座標を取得
    FVector GetAnchorLocation() const;

    //ワイヤー接続の切り替え
    void ToggleWire();

    // ワイヤー機動の開始・終了
    void AttachWire();
    void DetachWire();

    // アンカー接続先のアクタが削除された場合に発火
    UFUNCTION()
    void OnAttachedActorDestroyed(AActor* DestroyedActor);

    // ワイヤーを巻き取る
    void RetractWire();

    // ワイヤーを伸ばす
    void ExtendWire();


private:
    bool bIsWireAttached = false; // ワイヤーが接続されているか
    float CurrentWireLength = 0; // 現在のワイヤーの長さ
    AActor* AttachedActor; // 接続先のアクター（Movable の場合のみセット）
    UPrimitiveComponent* AttachedComponent; // 接続先のコンポーネント（Movable の場合のみセット）
    FVector StaticAnchorLocation; // Static なオブジェクトに接続した場合の固定座標
    UImage* CrosshairImage; // 生成したウィジェットのインスタンス
    bool bIsPrevConnectable; // 前フレームでワイヤーが接続可能だったか

    UPROPERTY(VisibleAnywhere, Category = "Wire")
    USceneComponent* AnchorComponent; // アンカーとして機能する SceneComponent（Movable 用）

    UPROPERTY(VisibleAnywhere, Category = "Wire")
    USplineMeshComponent* SplineMeshComponent; // Spline に沿ってメッシュを描画する

    UPROPERTY(EditAnywhere, Category = "Wire Settings")
    float WireRange = 10000.0f; // ワイヤーの射程距離

    UPROPERTY(EditAnywhere, Category = "Wire Settings")
    float WireMaxLength = 10000.0f; // ワイヤーの最大長

    UPROPERTY(EditAnywhere, Category = "Wire Settings")
    float RetractSpeed = 1500.0f; // ワイヤー巻き取り速度

    UPROPERTY(EditAnywhere, Category = "Wire Settings")
    float ExtendSpeed = 3000.0f; // ワイヤー伸ばし速度

    UPROPERTY(EditAnywhere, Category = "Sound Effect")
    UAudioComponent* WireAttachAudio; // ワイヤー接続時のオーディオ


    /** Returns CameraBoom subobject **/
    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    /** Returns FollowCamera subobject **/
    FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
