/*
  ==============================================================================

    MasterSearchPanel.h
    Created: Master search panel with position-based pad selection
    Author: Behzad Haki

  ==============================================================================
*/

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "CustomButtonStyle.h"

class SampleGridComponent; // Forward declaration

//==============================================================================
// Mini Pad Selection Button (represents a visual grid position)
//==============================================================================
class MiniPadButton : public Button
{
public:
    MiniPadButton(int row, int col);
    ~MiniPadButton() override;

    void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void setConnected(bool connected);
    bool isConnected() const { return isConnectedToMaster; }

    int getRow() const { return gridRow; }
    int getCol() const { return gridCol; }
    int getVisualPosition() const { return (3 - gridRow) * 4 + gridCol; } // Bottom-left = 0

private:
    int gridRow, gridCol;
    bool isConnectedToMaster;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiniPadButton)
};

//==============================================================================
// 4x4 Grid for selecting which pads connect to master search
//==============================================================================
class PadSelectionGrid : public Component
{
public:
    PadSelectionGrid();
    ~PadSelectionGrid() override;

    void paint(Graphics& g) override;
    void resized() override;

    void setPositionConnected(int row, int col, bool connected);
    bool isPositionConnected(int row, int col) const;

    void clearAllConnections();
    int getConnectedCount() const;

    std::function<void(int, int, bool)> onConnectionChanged;

private:
    static constexpr int GRID_SIZE = 4;
    std::array<std::array<std::unique_ptr<MiniPadButton>, GRID_SIZE>, GRID_SIZE> miniButtons;

    void handleButtonClicked(int row, int col);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PadSelectionGrid)
};

//==============================================================================
// Complete Master Search Panel
//==============================================================================
class MasterSearchPanel : public Component
{
public:
    MasterSearchPanel();
    ~MasterSearchPanel() override;

    void paint(Graphics& g) override;
    void resized() override;

    void setSampleGridComponent(SampleGridComponent* gridComponent);

    void setPositionConnected(int row, int col, bool connected);
    bool isPositionConnected(int row, int col) const;

    String getMasterQuery() const;
    void setMasterQuery(const String& query);

    std::function<void(const String&)> onMasterQueryChanged;
    std::function<void()> onSearchSelectedClicked;

private:
    SampleGridComponent* sampleGrid;

    PadSelectionGrid selectionGrid;
    TextEditor masterQueryEditor;
    StyledButton searchSelectedButton;
    StyledButton noneSelectionButton;
    StyledButton allSelectionButton;


    void updateSearchButtonState();
    void handleMasterQueryChanged();
    void handleSearchSelectedClicked();
    void handleActivateAll(bool activate_all);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterSearchPanel)
};