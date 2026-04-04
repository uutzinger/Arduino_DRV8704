/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "UUtzinger_DRV8704", "index.html", [
    [ "Purpose", "index.html#autotoc_md1", null ],
    [ "Hardware Model", "index.html#autotoc_md2", null ],
    [ "Required Wiring", "index.html#autotoc_md3", null ],
    [ "Quick Start", "index.html#autotoc_md4", null ],
    [ "Control Model", "index.html#autotoc_md5", [
      [ "Direction", "index.html#autotoc_md6", null ],
      [ "Static Bridge States", "index.html#autotoc_md7", null ],
      [ "Current Drive", "index.html#autotoc_md8", null ],
      [ "PWM With Current Limit", "index.html#autotoc_md9", null ]
    ] ],
    [ "Presets", "index.html#autotoc_md10", null ],
    [ "Mode Transitions", "index.html#autotoc_md11", null ],
    [ "Examples", "index.html#autotoc_md12", null ],
    [ "Example Snippets", "index.html#autotoc_md13", [
      [ "Current Drive", "index.html#autotoc_md14", null ],
      [ "PWM With Current Limit", "index.html#autotoc_md15", null ],
      [ "Coast And Brake", "index.html#autotoc_md16", null ]
    ] ],
    [ "PWM Backends", "index.html#autotoc_md17", null ],
    [ "Faults And Diagnostics", "index.html#autotoc_md18", null ],
    [ "Limitations", "index.html#autotoc_md19", null ],
    [ "Documentation", "index.html#autotoc_md20", null ],
    [ "Changelog", "md_CHANGELOG.html", [
      [ "Unreleased", "md_CHANGELOG.html#autotoc_md22", null ]
    ] ],
    [ "DRV8704 Driver TODO", "md_TODO.html", [
      [ "1. Project Skeleton", "md_TODO.html#autotoc_md24", null ],
      [ "2. Core Source Layout", "md_TODO.html#autotoc_md25", null ],
      [ "3. Register Definitions", "md_TODO.html#autotoc_md26", null ],
      [ "4. Types and Register Views", "md_TODO.html#autotoc_md27", null ],
      [ "5. Public API", "md_TODO.html#autotoc_md28", null ],
      [ "6. SPI Transport", "md_TODO.html#autotoc_md29", null ],
      [ "7. Lifecycle and Startup", "md_TODO.html#autotoc_md30", null ],
      [ "8. Core Register Configuration Layer", "md_TODO.html#autotoc_md31", null ],
      [ "9. Status and Fault Layer", "md_TODO.html#autotoc_md32", null ],
      [ "10. Runtime Bridge Control Layer", "md_TODO.html#autotoc_md33", null ],
      [ "11. Current Limit and Current Drive", "md_TODO.html#autotoc_md34", null ],
      [ "12. Predefined Load Presets", "md_TODO.html#autotoc_md35", null ],
      [ "13. PWM With Current Limit and Platform Backends", "md_TODO.html#autotoc_md36", null ],
      [ "14. Documentation", "md_TODO.html#autotoc_md37", null ],
      [ "15. Arduino Packaging", "md_TODO.html#autotoc_md38", null ],
      [ "16. Examples", "md_TODO.html#autotoc_md39", null ],
      [ "17. Validation", "md_TODO.html#autotoc_md40", null ],
      [ "18. Implementation Order", "md_TODO.html#autotoc_md41", null ]
    ] ],
    [ "DRV8704 Driver Requirements and Implementation Plan", "md_DRV8704_DRIVER_REQUIREMENTS.html", [
      [ "1. Purpose", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md43", null ],
      [ "2. Device Summary", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md44", null ],
      [ "3. Design Goals", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md45", null ],
      [ "4. Non-Goals", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md46", null ],
      [ "5. Functional Requirements", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md47", null ],
      [ "5.1 Initialization and Lifecycle", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md48", null ],
      [ "5.2 SPI Communication", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md49", null ],
      [ "5.3 Register Model", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md50", null ],
      [ "5.4 Core Register Configuration", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md51", null ],
      [ "5.5 Current Mode", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md52", null ],
      [ "5.6 Predefined Load Presets", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md53", null ],
      [ "5.7 PWM Input Mode and Platform PWM Generation", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md54", null ],
      [ "5.8 Runtime Bridge Control", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md55", null ],
      [ "5.9 Fault and Status Handling", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md56", null ],
      [ "5.10 Configuration Profiles", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md57", null ],
      [ "5.11 Diagnostics and Validation", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md58", null ],
      [ "5.12 Documentation Requirements", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md59", null ],
      [ "5.13 Documentation Scripts and Tooling", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md60", null ],
      [ "5.14 Arduino Library Packaging Requirements", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md61", null ],
      [ "6. API Requirements", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md62", null ],
      [ "7. Proposed Source Structure", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md63", [
        [ "7.1 File Responsibilities", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md64", null ]
      ] ],
      [ "8. Naming Requirements", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md65", null ],
      [ "9. Behavioral Requirements", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md66", null ],
      [ "10. Known Issues in Existing Attempt", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md67", null ],
      [ "11. Development Plan", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md68", null ],
      [ "Phase 1: Specification Freeze", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md69", null ],
      [ "Phase 2: Source Skeleton", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md70", null ],
      [ "Phase 3: SPI and Register Layer", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md71", null ],
      [ "Phase 4: Configuration Layer", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md72", null ],
      [ "Phase 5: Status and Fault Layer", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md73", null ],
      [ "Phase 6: Bridge Control Layer", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md74", null ],
      [ "Phase 7: Examples and Validation", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md75", null ],
      [ "12. Acceptance Criteria", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md76", null ],
      [ "13. Recommended Next Step", "md_DRV8704_DRIVER_REQUIREMENTS.html#autotoc_md77", null ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"drv8704__types_8h.html#af7fa5079f2831642b22404743565c06ea67d2f6740a8eaebf4d5c6f79be8da481"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';