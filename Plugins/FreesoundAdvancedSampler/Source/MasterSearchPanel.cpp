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
    , progressBar(currentProgress)

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
            // IMPORTANT: Don't clear query when disconnecting - let pad keep its individual query
        }
    };
    addAndMakeVisible(selectionGrid);

    // Master query editor
    masterQueryEditor.setMultiLine(false);
    masterQueryEditor.setReturnKeyStartsNewLine(false);
    masterQueryEditor.setScrollbarsShown(false);
    masterQueryEditor.setColour(TextEditor::backgroundColourId, Colour(0xff2A2A2A));
    masterQueryEditor.setColour(TextEditor::textColourId, Colours::white);
    masterQueryEditor.setColour(TextEditor::outlineColourId, Colour(0xff404040));           // Normal border
    masterQueryEditor.setColour(TextEditor::focusedOutlineColourId, Colour(0xff606060));   // Lighter grey when focused
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

    setupProgressComponents();

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
    int smallButtonWidth = 60;  // Smaller buttons for Detach/Attach All

    // Text editor at top of controls area
    auto lineHeight = controlsArea.getHeight() / 3; // Adjust height for text editor
    auto textArea = controlsArea.removeFromTop(lineHeight);
    searchSelectedButton.setBounds(textArea.removeFromRight(buttonWidth));
    textArea.removeFromRight(buttonSpacing); // spacing after search button
    masterQueryEditor.setBounds(textArea);

    controlsArea.removeFromTop(3); // spacing

    // Bottom area: split between buttons and progress
    auto bottomArea = controlsArea;

    // Left side: Detach/Attach All buttons (stacked vertically)
    auto buttonArea = bottomArea.removeFromLeft(smallButtonWidth);
    int buttonHeight = 20;
    noneSelectionButton.setBounds(buttonArea.removeFromTop(buttonHeight));
    buttonArea.removeFromTop(3); // small spacing between buttons
    allSelectionButton.setBounds(buttonArea.removeFromTop(buttonHeight));

    bottomArea.removeFromLeft(buttonSpacing); // spacing after buttons

    // Right side: Progress components (stacked vertically)
    if (progressBar.isVisible())
    {
        auto progressArea = bottomArea;
        int progressLineHeight = 14;

        statusLabel.setBounds(progressArea.removeFromTop(progressLineHeight));
        progressArea.removeFromTop(2); // small spacing
        progressBar.setBounds(progressArea.removeFromTop(progressLineHeight));
        progressArea.removeFromTop(2); // small spacing
        cancelButton.setBounds(progressArea.removeFromTop(progressLineHeight));
    }
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

                // Only sync master query when activating, not when deactivating
                if (activate_all)
                {
                    sampleGrid->syncMasterQueryToPosition(row, col, getMasterQuery());
                }
                // When deactivating (activate_all = false), don't touch the queries
            }
        }
    }

    updateSearchButtonState();
}

void MasterSearchPanel::setupProgressComponents()
{
    // Set up progress bar
    progressBar.setColour(ProgressBar::backgroundColourId, Colour(0xff404040));
    progressBar.setColour(ProgressBar::foregroundColourId, Colour(0xff00D9FF));
    addAndMakeVisible(progressBar);

    // Set up status label
    statusLabel.setText("Ready", dontSendNotification);
    statusLabel.setFont(Font(9.0f));
    statusLabel.setColour(Label::textColourId, Colours::white);
    statusLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(statusLabel);

    // Set up cancel button
    cancelButton.setButtonText("Cancel");
    cancelButton.setEnabled(false);
    cancelButton.onClick = [this]()
    {
        if (sampleGrid && sampleGrid->getProcessor())
        {
            sampleGrid->getProcessor()->cancelDownloads();
            cancelButton.setEnabled(false);
            statusLabel.setText("Download cancelled", dontSendNotification);
        }
    };
    addAndMakeVisible(cancelButton);

    // Initially hide progress components
    updateProgressVisibility(false);
}

void MasterSearchPanel::updateProgressVisibility(bool visible)
{
    progressBar.setVisible(visible);
    statusLabel.setVisible(visible);
    cancelButton.setVisible(visible);
}

void MasterSearchPanel::showProgress(bool show)
{
    updateProgressVisibility(show);
    resized(); // Trigger layout update
}

void MasterSearchPanel::updateDownloadProgress(const AudioDownloadManager::DownloadProgress& progress)
{
    MessageManager::callAsync([this, progress]()
    {
        currentProgress = progress.overallProgress;

        String statusText = "Downloading " + progress.currentFileName +
                           " (" + String(progress.completedFiles) +
                           "/" + String(progress.totalFiles) + ") - " +
                           String((int)(progress.overallProgress * 100)) + "%";

        statusLabel.setText(statusText, dontSendNotification);
        showProgress(true);
        cancelButton.setEnabled(true);
        progressBar.repaint();
    });
}

void MasterSearchPanel::downloadCompleted(bool success)
{
    MessageManager::callAsync([this, success]()
    {
        cancelButton.setEnabled(false);

        statusLabel.setText(success ? "Download complete!" :
                          "Download failed",
                          dontSendNotification);

        currentProgress = success ? 1.0 : 0.0;
        progressBar.repaint();

        if (success && sampleGrid)
        {
            Timer::callAfterDelay(500, [this]()
            {
                if (sampleGrid->hasPendingMasterSearch())
                {
                    sampleGrid->populatePadsFromMasterSearch();
                }
            });
        }

        // Hide progress components after a delay
        Timer::callAfterDelay(2000, [this]()
        {
            showProgress(false);
        });
    });
}