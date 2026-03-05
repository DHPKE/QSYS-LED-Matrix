# Q-SYS LED Matrix Plugin Changelog

## Version 7.1.0 (2026-03-03)
### Added Tabbed UI
- Implemented `GetPages()` returning 5 tab names: ["Connection", "Segments", "Layout", "Global", "Curtain"]
- Refactored `GetControlLayout()` to conditionally render controls based on `props["page_index"].Value`
- All controls remain in `GetControls()` for persistent state across tabs

### Tab Organization
- **Page 0 (Connection)**: IP, port, status, reconnect, last command
- **Page 1 (Segments)**: 4 segment rows with text/color/font/size/align/effect controls
- **Page 2 (Layout)**: Group routing buttons (All + 1-8), preset layout thumbnails + dropdown
- **Page 3 (Global)**: Brightness, rotation, display enable, test mode, reboot, clear all
- **Page 4 (Curtain)**: Group field, color dropdown, Apply button, Enable toggle

### Technical Details
- No functionality changes from v7.0.5 - all features work identically
- UI is organized into logical sections for better usability
- Each page renders only relevant controls while maintaining state

### Testing Checklist
- [ ] Plugin loads without errors in Q-SYS Designer
- [ ] All 5 tabs are visible and switch correctly
- [ ] Connection tab: IP/port changes work, reconnect button functions
- [ ] Segments tab: All 4 segments render, text/color/font controls work
- [ ] Layout tab: Group selection works, layout presets apply correctly
- [ ] Global tab: Brightness, rotation, display enable, test mode, reboot all functional
- [ ] Curtain tab: Group field, color dropdown, apply and enable buttons work
- [ ] All functionality from v7.0.5 works identically

## Version 7.0.5 (Previous)
- Fix enable button (show/hide) + proportional segment scaling
- All curtain features functional
