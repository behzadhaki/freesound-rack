/*
  ==============================================================================

    MasterSearchPanel.cpp
    Created: Master search panel with position-based pad selection
    Author: Generated

  ==============================================================================
*/

#include "MasterSearchPanel.h"
#include "SampleGridComponent.h"

//==============================================================================
// MiniPadButton Implementation
//==============================================================================

MiniPadButton::MiniPadButton(int row, int col)
    : Button(""), gridRow(row), gridCol(col), isConnectedToMaster(false)
{
    setSize(20, 20);
    setClickingTogglesState(true);
}

MiniPadButton::~MiniPadButton() {}

void MiniPadButton::paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat();

    // Background color based on connection state
    Colour bgColour;
    if (isConnectedToMaster)
    {
        bgColour = Colour(0xff00D9FF).withAlpha(0.8f); // Bright blue for connected
    }
    else
    {
        bgColour = Colour(0xff404040).withAlpha(0.6f); // Dark gray for disconnected
    }

    if (shouldDrawButtonAsHighlighted)
        bgColour = bgColour.brighter(0.2f);
    if (shouldDrawButtonAsDown)
        bgColour = bgColour.darker(0.2f);

    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, 3.0f);

    // Border
    g.setColour(isConnectedToMaster ? Colours::white : Colour(0xff666666));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);

    // Position number (1-16, bottom-left = 1)
    g.setColour(isConnectedToMaster ? Colours::white : Colour(0xff999999));
    g.setFont(Font(8.0f, Font::bold));

    int positionNumber = getVisualPosition() + 1; // 1-based for display
    g.drawText(String(positionNumber), bounds.toNearestInt(), Justification::centred);
}

void MiniPadButton::setConnected(bool connected)
{
    if (isConnectedToMaster != connected)
    {
        isConnectedToMaster = connected;
        setToggleState(connected, dontSendNotification);
        repaint();
    }
}

//==============================================================================
// PadSelectionGrid Implementation
//==============================================================================

PadSelectionGrid::PadSelectionGrid()
{
    // Create 4x4 grid of mini buttons
    for (int row = 0; row < GRID_SIZE; ++row)
    {
        for (int col = 0; col < GRID_SIZE; ++col)
        {
            miniButtons[row][col] = std::make_unique<MiniPadButton>(row, col);

            miniButtons[row][col]->onClick = [this, row, col]() {
                handleButtonClicked(row, col);
            };

            addAndMakeVisible(*miniButtons[row][col]);
        }
    }
}

PadSelectionGrid::~PadSelectionGrid() {}

void PadSelectionGrid::paint(Graphics& g)
{
    auto bounds = getLocalBounds();

    // Dark background
    g.setColour(Colour(0xff1A1A1A).withAlpha(0.8f));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border
    g.setColour(Colour(0xff404040));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, 1.0f);
}

void PadSelectionGrid::resized()
{
    auto bounds = getLocalBounds().reduced(4);

    int buttonSize = jmin(
        (bounds.getWidth() - 3 * 2) / GRID_SIZE,  // 3 * 2 = spacing between buttons
        (bounds.getHeight() - 3 * 2) / GRID_SIZE
    );

    buttonSize = jmax(buttonSize, 16); // Minimum size

    int totalWidth = buttonSize * GRID_SIZE + 2 * 3;
    int totalHeight = buttonSize * GRID_SIZE + 2 * 3;

    int startX = bounds.getX() + (bounds.getWidth() - totalWidth) / 2;
    int startY = bounds.getY() + (bounds.getHeight() - totalHeight) / 2;

    for (int row = 0; row < GRID_SIZE; ++row)
    {
        for (int col = 0; col < GRID_SIZE; ++col)
        {
            int x = startX + col * (buttonSize + 2);
            int y = startY + row * (buttonSize + 2);

            miniButtons[row][col]->setBounds(x, y, buttonSize, buttonSize);
        }
    }
}

void PadSelectionGrid::setPositionConnected(int row, int col, bool connected)
{
    if (row >= 0 && row < GRID_SIZE && col >= 0 && col < GRID_SIZE)
    {
        miniButtons[row][col]->setConnected(connected);
    }
}

bool PadSelectionGrid::isPositionConnected(int row, int col) const
{
    if (row >= 0 && row < GRID_SIZE && col >= 0 && col < GRID_SIZE)
    {
        return miniButtons[row][col]->isConnected();
    }
    return false;
}

void PadSelectionGrid::clearAllConnections()
{
    for (int row = 0; row < GRID_SIZE; ++row)
    {
        for (int col = 0; col < GRID_SIZE; ++col)
        {
            setPositionConnected(row, col, false);
        }
    }
}

void PadSelectionGrid::activateAllConnections()
{
    for (int row = 0; row < GRID_SIZE; ++row)
    {
        for (int col = 0; col < GRID_SIZE; ++col)
        {
            setPositionConnected(row, col, true);
        }
    }
}

int PadSelectionGrid::getConnectedCount() const
{
    int count = 0;
    for (int row = 0; row < GRID_SIZE; ++row)
    {
        for (int col = 0; col < GRID_SIZE; ++col)
        {
            if (isPositionConnected(row, col))
                count++;
        }
    }
    return count;
}

void PadSelectionGrid::handleButtonClicked(int row, int col)
{
    bool newState = !isPositionConnected(row, col);
    setPositionConnected(row, col, newState);

    if (onConnectionChanged)
        onConnectionChanged(row, col, newState);
}

//==============================================================================
// MasterSearchPanel Implementation
//==============================================================================

MasterSearchPanel::MasterSearchPanel()
    : sampleGrid(nullptr)
    , searchSelectedButton("Search Selected", 9.0f, false)
    , noneSelectionButton("Detach All", 9.0f, false)
    , allSelectionButton("Attach All", 9.0f, false)
{
    // Selection grid
    selectionGrid.onConnectionChanged = [this](int row, int col, bool connected) {
        updateSearchButtonState();

        // Update the main grid pads
        if (sampleGrid)
        {
            sampleGrid->setPositionConnectedToMaster(row, col, connected);

            // If connecting, sync the current master query to this position
            if (connected)
            {
                sampleGrid->syncMasterQueryToPosition(row, col, getMasterQuery());
            }
        }
    };
    addAndMakeVisible(selectionGrid);

    // Master query editor
    masterQueryEditor.setMultiLine(false);
    masterQueryEditor.setReturnKeyStartsNewLine(false);
    masterQueryEditor.setScrollbarsShown(false);
    masterQueryEditor.setColour(TextEditor::backgroundColourId, Colour(0xff2A2A2A));
    masterQueryEditor.setColour(TextEditor::textColourId, Colours::white);
    masterQueryEditor.setColour(TextEditor::outlineColourId, Colour(0xff404040));
    masterQueryEditor.setColour(TextEditor::focusedOutlineColourId, Colour(0xff00D9FF));
    masterQueryEditor.setFont(Font(10.0f));
    masterQueryEditor.setTextToShowWhenEmpty("Enter search query for connected pads...", Colours::grey);

    masterQueryEditor.onTextChange = [this]() {
        handleMasterQueryChanged();
    };

    masterQueryEditor.onReturnKey = [this]() {
        if (searchSelectedButton.isEnabled())
            handleSearchSelectedClicked();
    };

    addAndMakeVisible(masterQueryEditor);

    // Search selected button
    searchSelectedButton.onClick = [this]() {
        handleSearchSelectedClicked();
    };
    addAndMakeVisible(searchSelectedButton);

    // None and All selections buttons
    noneSelectionButton.onClick = [this]() {
        handleActivateAll(false);
    };
    addAndMakeVisible(noneSelectionButton);

    allSelectionButton.onClick = [this]() {
        handleActivateAll(true);
    };
    addAndMakeVisible(allSelectionButton);

    updateSearchButtonState();
}

MasterSearchPanel::~MasterSearchPanel() {}

void MasterSearchPanel::paint(Graphics& g)
{
    auto bounds = getLocalBounds();

    // Modern dark background
    g.setColour(Colour(0xff1A1A1A).withAlpha(0.9f));
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    // Subtle border
    g.setColour(Colour(0xff404040).withAlpha(0.6f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 1.0f);
}

void MasterSearchPanel::resized()
{
    auto bounds = getLocalBounds().reduced(6);

    // Main content area organized left to right
    auto contentArea = bounds;

    // Left side: Selection grid (square area)
    int gridSize = jmin(contentArea.getWidth() / 3, contentArea.getHeight());
    auto gridArea = contentArea.removeFromLeft(gridSize);
    selectionGrid.setBounds(gridArea);

    contentArea.removeFromLeft(8); // spacing between grid and controls

    // Right side: Text editor and buttons (vertical stack)
    auto controlsArea = contentArea;

    // Buttons below text editor (horizontal layout)
    int buttonWidth = 80;
    int buttonSpacing = 6;

    // Text editor at top of controls area
    auto LineHeight = contentArea.getHeight() / 3; // Adjust height for text editor
    auto textArea = controlsArea.removeFromTop(LineHeight);
    searchSelectedButton.setBounds(textArea.removeFromRight(buttonWidth));
    textArea.removeFromRight(buttonSpacing); // spacing after search button
    masterQueryEditor.setBounds(textArea);

    controlsArea.removeFromTop(3); // spacing

    controlsArea.removeFromTop(3); // spacing above buttons
    auto buttonArea = controlsArea.removeFromLeft(buttonWidth);
    noneSelectionButton.setBounds(buttonArea.removeFromTop(20));
    allSelectionButton.setBounds(buttonArea.removeFromBottom(20));
    // buttonArea.removeFromLeft(buttonSpacing);

}

void MasterSearchPanel::setSampleGridComponent(SampleGridComponent* gridComponent)
{
    sampleGrid = gridComponent;
}

void MasterSearchPanel::setPositionConnected(int row, int col, bool connected)
{
    selectionGrid.setPositionConnected(row, col, connected);
    updateSearchButtonState();
}

bool MasterSearchPanel::isPositionConnected(int row, int col) const
{
    return selectionGrid.isPositionConnected(row, col);
}

String MasterSearchPanel::getMasterQuery() const
{
    return masterQueryEditor.getText().trim();
}

void MasterSearchPanel::setMasterQuery(const String& query)
{
    masterQueryEditor.setText(query, dontSendNotification);
}

void MasterSearchPanel::updateSearchButtonState()
{
    bool hasConnections = selectionGrid.getConnectedCount() > 0;
    bool hasQuery = getMasterQuery().isNotEmpty();

    searchSelectedButton.setEnabled(hasConnections && hasQuery);
}

void MasterSearchPanel::handleMasterQueryChanged()
{
    String newQuery = getMasterQuery();

    // Sync query to all connected positions in real-time
    if (sampleGrid)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                if (isPositionConnected(row, col))
                {
                    sampleGrid->syncMasterQueryToPosition(row, col, newQuery);
                }
            }
        }
    }

    updateSearchButtonState();

    if (onMasterQueryChanged)
        onMasterQueryChanged(newQuery);
}

void MasterSearchPanel::handleSearchSelectedClicked()
{
    if (onSearchSelectedClicked)
        onSearchSelectedClicked();
}

void MasterSearchPanel::handleActivateAll(bool activate_all)
{
    if (activate_all)
        selectionGrid.activateAllConnections();
    else
        selectionGrid.clearAllConnections();

    // Update the main grid
    if (sampleGrid)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                sampleGrid->setPositionConnectedToMaster(row, col, activate_all);
            }
        }
    }

    updateSearchButtonState();
}