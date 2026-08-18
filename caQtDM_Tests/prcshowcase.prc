# prcshowcase.prc -- synthetic showcase covering most of the .prc capabilities.
# Uses the standard channels of the caQtDM_Tests simulation IOC
# (mySimulation.db, start with run-ioc / st.cmd), so most widgets show
# live values. Render with:  prc2ui prcshowcase.prc   or open
# directly in caQtDM with CAQTDM_PRC_CONVERTER=1.
# The old parser ignores the #!tab directive and renders everything flat.
#
# $Author: synthesized from pep.tcl (Werner Portmann) semantics $
#!title "PRC Showcase (simulation IOC)"
#!qtbg #d8e0d8
#!grid 2
#!setup SHOWCASE-REQ
#channel name                    type        args
#
# ---------------------------------------------------------------- tab 1
#!tab "Monitors"
comment comment -span 2 -comsize 16 -comfg "#003080" Formatted readbacks
ACM:TEST:AO:NOISE                formRead 9.3 -text "noisy value"
ACM:TEST:AO:NOISE                formRead 9.2e -text "exponential"
ACM:COUNT:1                      formRead %8.1f -text "counter printf"
ACM:TEST:LI                      formRead x -text "hexadecimal"
mystring                         formRead s -text "string value"
ACM:TEST:AI                      formRead 6.2 -desc
ACM:TEST:EGU:1                   formRead 7.2 -text "custom unit" -cegu " mbar"
ACM:COUNT:1                      formRead 6.2 -text "calc value/60" -calc "value/60" -cegu " min"
ACM:TEST:AO                      formRead 9.3 -text "wide field" -minwidth 160 -span 2
LI:OP:demoAI                     formRead 9.3 -text "with panel" -comlab "Panel..." -command "caqtdm tests.ui"
comment comment -span 2 -comsize 14 -comfg "#003080" Leds and status
ACM:TEST:BI                      led -text "alarm ring led"
ACM:TEST:BO3                     led -ledstate "0 green 1 red" -text "two states"
ACM:TEST:MBBO                    led -ledstate "0 gray 1 green 2 yellow 3 red" -text "four states stacked"
ACM:TEST:BO1                     led -visi "ACM:TEST:BO1==1" -text "visible if BO1=1"
ACM:TEST:BI                      compare -text "compare sign"
comment comment -span 2 -comsize 14 -comfg "#003080" Bars
ACM:TEST:AO:NOISE                bar -OPR -10 10 -text "noise -10..10" -printvar 5.1f
ACM:TEST:AO                      bar -OPR 0 10 -text "user limits" -printvar 4.1f -cegu " A"
#
# ---------------------------------------------------------------- tab 2
#!tab "Controls"
comment comment -span 2 -comsize 16 -comfg "#803000" Setting values
ACM:TEST:EGU:1                   wheelSwitch 7.3 -text "wheel default"
ACM:TEST:EGU:2                   wheelSwitch 6.2 -OPR -5 5 -text "wheel with limits"
comment comment -span 2 "setRdbk derives :I-COMP/:I-READ/:ONOFF/:PS-MODE from the device name (simulated by the SIMPS records: ONOFF gates the readback)"
SIMPS-1:I-SET                    setRdbk 9.3 9.3
SIMPS-2:I-SET                    mactrl 7.2 -text "mactrl alias" -hys -confirm
comment comment -span 2 "the same device built from single widgets (SIMPS-2, interacts with the mactrl row above):"
SIMPS-2:I-SET                    wheelSwitch 7.3 -text "set current"
SIMPS-2:I-READ                   formRead 7.3 -text "readback"
SIMPS-2:ONOFF                    choiceButton -text "on/off"
SIMPS-2:PS-MODE                  formRead s -text "mode"
comment comment -span 2 -comsize 14 -comfg "#803000" Discrete controls
ACM:TEST:BO                      binary -text "toggle with readback"
ACM:TEST:BO2                     binary -compact -text "toggle only"
ACM:TEST:MBBO                    menuButton -text "enum menu"
ACM:TEST1:MBBO                   choiceButton -text "enum buttons"
ACM:TEST:BO2                     messagebutton -label "GO" 1
comment comment -span 2 -comsize 14 -comfg "#803000" Analog controls
99:SliderAO                      slider 0.5 -text "slider step 0.5"
ACM:TEST:AO:AMP                  slider -trough 6.2 -text "slider + readback"
ACM:MOTOR                        slider 0.1 -OPR 0 100 -text "slider 0..100"
comment comment -span 2 -comsize 14 -comfg "#803000" Text input
mystring                         text -text "string with entry"
bigstring01                      entry -width 14 -text "plain entry"
#
# ---------------------------------------------------------------- tab 3
#!tab "Layout"
comment comment -span 2 -comsize 16 -comfg darkgreen Quoting and comments
comment comment -span 2 "double quoted   text keeps   blanks"
comment comment -span 2 'single quoted text with "double quotes" inside'
comment comment -span 2 {braced text with "quotes" and {nested braces} inside}
comment comment -comjust center -bg "#ffffc0" centered on yellow
comment comment -comjust right -fg red right adjusted
comment comment -span 2 -command {exec caqtdm "tests.ui"} -comlab "Related..." comment with command button
separator separator -span 2
comment comment -span 2 "separators: -height / -linewidth / -sepsize / -sepbg (quoted, so not parsed as options)"
separator separator -span 2 -height 6
separator separator -span 2 -linewidth 3 -fg "#4060c0"
separator separator -span 2 -sepsize 8 -sepbg orange
comment comment -span 2 spans and empty cells
ACM:TEST:AO:NOISE                formRead 9.3 -text "span 2" -span 2
ACM:TEST:BI                      led -notext
comment comment
ACM:COUNT:1                      formRead 9.1 -notext
