/**
 * HUDProgress - Persistent in-game XP progress bar overlay
 *
 * Receives progress data from C++ via window.onHUDProgressUpdate and renders
 * per-school progress bars as a draggable HTML/CSS overlay.
 *
 * Features:
 *   - One progress bar per school with spell name + XP percentage
 *   - School colors from settings.schoolColors
 *   - Drag-to-reposition (only when hudProgress.editMode is active)
 *   - pointer-events: none during normal viewing (does not block gameplay)
 *   - Position/scale/opacity persisted via unified config
 */

var HUDProgress = (function() {
    var _overlay = null;
    var _container = null;
    var _titleBar = null;
    var _isDragging = false;
    var _dragOffset = { x: 0, y: 0 };
    var _panelVisible = false;

    // ------------------------------------------------------------------
    // Init - create DOM elements if not already present
    // ------------------------------------------------------------------
    function init() {
        if (_overlay) return;

        // Check if overlay already exists in HTML (e.g., hud.html)
        _overlay = document.getElementById('hudProgressOverlay');
        if (!_overlay) {
            _overlay = document.createElement('div');
            _overlay.id = 'hudProgressOverlay';
            _overlay.className = 'hud-progress-overlay';

            _container = document.createElement('div');
            _container.className = 'hud-progress-container';

            _titleBar = document.createElement('div');
            _titleBar.className = 'hud-progress-title';
            _titleBar.textContent = 'Spell Learning Progress';

            _overlay.appendChild(_titleBar);
            _overlay.appendChild(_container);
            document.body.appendChild(_overlay);
        } else {
            _container = _overlay.querySelector('.hud-progress-container');
            _titleBar = _overlay.querySelector('.hud-progress-title');
        }

        applySettings();
        setupDragHandlers();
        setupEscapeHandler();

        // Do not force display:none here — applySettings() handles visibility
        // based on settings.hudProgress.enabled. Default hidden until config loads.
        if (!settings.hudProgress || !settings.hudProgress.enabled) {
            _overlay.style.display = 'none';
        }
        // pointerEvents is set by applySettings() based on editMode — don't override here
    }

    // ------------------------------------------------------------------
    // Apply settings (position, scale, opacity, visibility)
    // ------------------------------------------------------------------
    function applySettings() {
        if (!_overlay || !settings.hudProgress) return;

        var hp = settings.hudProgress;

        _overlay.style.display = hp.enabled ? 'block' : 'none';
        _overlay.style.left = hp.position.x + 'px';
        _overlay.style.top = hp.position.y + 'px';
        _overlay.style.transform = 'scale(' + (hp.scale / 100) + ')';
        _overlay.style.opacity = (hp.opacity / 100).toString();

        // Overlay never hijacks input — stays pointer-events: none so it never
        // blocks gameplay or the settings toggle. Only the title bar becomes
        // interactive (draggable) when edit mode is on.
        _overlay.style.pointerEvents = 'none';
        var allowPointer = hp.editMode;
        _titleBar.style.pointerEvents = allowPointer ? 'auto' : 'none';
        _titleBar.style.cursor = allowPointer ? 'grab' : 'default';
    }

    // ------------------------------------------------------------------
    // Toggle HUD on/off
    // ------------------------------------------------------------------
    function setEnabled(enabled) {
        if (!settings.hudProgress) return;
        settings.hudProgress.enabled = enabled;
        applySettings();
        if (enabled) {
            // Request immediate data from C++
            if (window.callCpp) {
                window.callCpp('GetHUDProgress', '');
            }
        }
    }

    // ------------------------------------------------------------------
    // Set edit mode (allows dragging). Saves to settings so it persists.
    // ------------------------------------------------------------------
    function setEditMode(edit) {
        if (!settings.hudProgress) return;
        settings.hudProgress.editMode = !!edit;
        applySettings();
    }

    // ------------------------------------------------------------------
    // Update position from settings
    // ------------------------------------------------------------------
    function setPosition(x, y) {
        if (!settings.hudProgress) return;
        settings.hudProgress.position.x = x;
        settings.hudProgress.position.y = y;
        applySettings();
    }

    // ------------------------------------------------------------------
    // Handle progress data from C++
    // ------------------------------------------------------------------
    function onProgressUpdate(dataStr) {
        if (!settings.hudProgress || !settings.hudProgress.enabled) return;

        var data;
        try {
            data = typeof dataStr === 'string' ? JSON.parse(dataStr) : dataStr;
        } catch (e) {
            console.error('[HUDProgress] Failed to parse progress data:', e);
            return;
        }

        if (!data || !data.schools) return;

        _container.innerHTML = '';

        data.schools.forEach(function(entry) {
            if (!entry.school || !entry.name) return;

            var bar = document.createElement('div');
            bar.className = 'hud-progress-bar';

            var schoolColor = (settings.schoolColors && settings.schoolColors[entry.school])
                ? settings.schoolColors[entry.school]
                : '#ffffff';

            var pct = Math.round((entry.progressPercent || 0) * 100);

            // School label + spell name
            var label = document.createElement('div');
            label.className = 'hud-progress-label';
            label.textContent = entry.school + ' — ' + entry.name;
            label.style.color = schoolColor;

            // Bar row: track + percentage on the same line below label
            var barRow = document.createElement('div');
            barRow.className = 'hud-progress-row';

            // Progress bar track
            var track = document.createElement('div');
            track.className = 'hud-progress-track';

            // Progress bar fill
            var fill = document.createElement('div');
            fill.className = 'hud-progress-fill';
            fill.style.width = pct + '%';
            fill.style.backgroundColor = schoolColor;

            track.appendChild(fill);

            // Percentage text on same line as bar
            var pctText = document.createElement('div');
            pctText.className = 'hud-progress-pct';
            pctText.textContent = pct + '%';

            barRow.appendChild(track);
            barRow.appendChild(pctText);

            bar.appendChild(label);
            bar.appendChild(barRow);

            _container.appendChild(bar);
        });
    }

    // ------------------------------------------------------------------
    // Drag-to-reposition handlers
    // ------------------------------------------------------------------
    function setupDragHandlers() {
        if (!_overlay || !_titleBar) return;

        _titleBar.addEventListener('mousedown', function(e) {
            if (!settings.hudProgress || !settings.hudProgress.editMode) return;
            _isDragging = true;
            _dragOffset.x = e.clientX - _overlay.offsetLeft;
            _dragOffset.y = e.clientY - _overlay.offsetTop;
            _overlay.style.cursor = 'grabbing';
            e.preventDefault();
        });

        window.addEventListener('mousemove', function(e) {
            if (!_isDragging) return;
            var x = e.clientX - _dragOffset.x;
            var y = e.clientY - _dragOffset.y;
            setPosition(x, y);
        });

        window.addEventListener('mouseup', function() {
            if (_isDragging) {
                _isDragging = false;
                _overlay.style.cursor = (settings.hudProgress && settings.hudProgress.editMode) ? 'grab' : 'default';
                // Debounce settings save - handled by settings autosave
            }
        });

        _titleBar.style.cursor = (settings.hudProgress && settings.hudProgress.editMode) ? 'grab' : 'default';
    }

    // ------------------------------------------------------------------
    // Escape key exits edit mode (escape hatch if HUD blocks the toggle)
    // ------------------------------------------------------------------
    function setupEscapeHandler() {
        window.addEventListener('keydown', function(e) {
            if (e.key === 'Escape' && settings.hudProgress && settings.hudProgress.editMode) {
                setEditMode(false);
            }
        });
    }

    // ------------------------------------------------------------------
    // Exposed API
    // ------------------------------------------------------------------
    return {
        init: init,
        setEnabled: setEnabled,
        setEditMode: setEditMode,
        setPosition: setPosition,
        applySettings: applySettings,
        onProgressUpdate: onProgressUpdate
    };
})();

// Wire into C++ callback
window.onHUDProgressUpdate = function(dataStr) {
    HUDProgress.onProgressUpdate(dataStr);
};

// Wire panel visibility from C++ - drag only allowed when panel is open
window.onPanelVisibilityChanged = function(visibleStr) {
    _panelVisible = (visibleStr === 'true' || visibleStr === true);
    applySettings();
};
