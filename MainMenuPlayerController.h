#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuPlayerController.generated.h"

class UMainMenuWidget;

UCLASS()
class PORTAL_API AMainMenuPlayerController : public APlayerController {
    GENERATED_BODY()

public:
    AMainMenuPlayerController();

protected:
    virtual void BeginPlay() override;

public:
    // Main Menu Management
    UFUNCTION(BlueprintCallable, Category = "Main Menu")
    void ShowMainMenu();

    UFUNCTION(BlueprintCallable, Category = "Main Menu")
    void HideMainMenu();

    UFUNCTION(BlueprintPure, Category = "Main Menu")
    UMainMenuWidget* GetMainMenuWidget() const { return MainMenuWidget; }

protected:
    // Widget Configuration
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UMainMenuWidget> MainMenuWidgetClass;

    // Current Widget
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    TObjectPtr<UMainMenuWidget> MainMenuWidget;

private:
    void SetupInputMode();
    void CreateMainMenuWidget();
};