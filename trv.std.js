// shortcuts for parts of $ object
var $t = $.trace;
var $v = $.view;
function $p(p) { $.print(p); }

// stores last filter created with vf call
var $f = null;

// add filter 
function vf(coll, color, title) 
{
    $f = $.view.filter(coll, color, title); 
}
function vfi(coll, color, title) { $.view.filterinteractive(coll, color, title); }

function vr(id) { $.view.removefilter(id); }

// show/hide unselected text
function vt() { $.view.showfilters = !$.view.showfilters; }
$.dotexpressions.add("f", vt);

// print filters
function pf() { $.view.printfilters(); }
$.dotexpressions.add("pf", pf);


function vsw(condition, color, title) 
{ 
    vf($.trace.where(condition), color, title); 
}
$.dotexpressions.add("a", vsw);

function vd(id) { $.view.enablefilter(id, false); }
function ve(id) { $.view.enablefilter(id, true); }

// add current line to quick collection
function ql() { quick.addline($.view.currentline); }
$.dotexpressions.add("ql", ql);

// move cursor to console and add .a in beginning
function startEditFilter()
{
    $.console.setFocus();
    $.console.setText(".a ");
}
$.shortcuts.add("ctrl+a", startEditFilter);

// bind to keyboard here
// $.keybinding.add("Ctrl+H", vt())

// history
var $h = $.history;
function hp() { $.history.print(); }
function he(id) { $.history.exec(id); }

// Trace line: [2012-11-15 08:31:30] message here
// $t.format = "\\[?year(\\d+)-?month(\\d+)-?day(\\d+) ?hour(\\d+):?minute(\\d+):?second(\\d+)\\] (.*)";

// 06/22/2012-09:17:52.69610060 [14B0] 06/22/2012-09:17:52.69610060 [14B0]    File was created: 
// $t.format = "?month(\\d+)/?day(\\d+)/?year(\\d+)-?hour(\\d+):?minute(\\d+):?second(\\d+).?nanosecond(\\x+) \\[?tid(\\x+)\\]\\s*(.*)";
$t.format = "MSI \\(s\\) \\(?tid(\\d+):(.*)"

$v.setviewlayout(30, 0.3);

var Black = "Black";
var DarkBlue = "DarkBlue";
var DarkGreen = "DarkGreen";
var DarkCyan = "DarkCyan";
var DarkRed = "DarkRed";
var DarkMagenta = "DarkMagenta";
var Brown = "Brown";
var LightGray = "LightGray";
var DarkGray = "DarkGray";
var Blue = "Blue";
var Green = "Green";
var Cyan = "Cyan";
var Red = "Red";
var Magenta = "Magenta";
var Yellow = "Yellow";
var White = "White";

$.import("trv.test.js");
$.import("trv.help.js");
