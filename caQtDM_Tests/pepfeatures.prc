# pepfeatures.prc -- test panel covering the pep.tcl features found in real
# SLS panels. Used with prc2ui to show what the caQtDM prc interpretation
# supports and what it silently drops (regression reference for improvements).
#!title "pep feature test panel"
#!grid 2
#channel name                     type   args
comment comment -span 2 -fg "#0000ff" -comsize 14 widgets supported by caQtDM today
$(P):AI-1                         formRead 9.3 -text "formread with format"
$(P):AI-2                         formRead -desc
$(P):I-SET                        setRdbk 9.3 9.3
$(P):I-SET2                       setRdbk 9.3 9.3 -hys -confirm
$(P):WS-1                         wheelSwitch 7.4 -OPR -10 10
$(P):BI-1                         binary -text "binary toggle"
$(P):BI-2                         binary -compact -notext
$(P):LED-1                        led -ledstate "0 green 1 red"
$(P):LED-2                        led -visi "$(P):BI-1==1"
$(P):MB-1                         menuButton -text "menu"
$(P):CB-1                         choiceButton -text "choice"
$(P):TXT-1                        text -text "long text"
separator separator -span 2 -height 4 -linewidth 2 -fg darkgrey
comment comment -span 2 -fg "#ff0000" -comsize 14 widgets missing in caQtDM (pep.tcl only)
$(P):SL-1                         slider 0.2
$(P):SL-2                         slider -trough 6.2
$(P):BAR-1                        bar -text "bar chart" -printvar 4.1f
$(P):EN-1                         entry -width 12
$(P):CMP-1                        compare
$(P):I-SET3                       mactrl 9.3 9.3
$(P):MSG-1                        messagebutton -label "GO" 1 -confirm
separator separator -span 2 -height 4
comment comment -span 2 related display and command -command "echo test" -comlab "run"
$(P):AI-3                         formRead 9.3 -command "echo related" -comlab "panel"
