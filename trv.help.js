function help()
{
    $.print("Trv.js is a trace viewer which provides extensive filtering capabilities by utilizing jscript runtime.");
    $.print("For example running following command will cause lines which match filter be highlighter with Red color");
    $.print("  vsw(\"my line\", Red, \"comment\")");
    $.print("The list of current filters can be viewed by running");
    $.print("  vp()");
    $.print("You can see full list of commands in std.js");
    $.print("Keyboard");
    $.print("  Ctrl+H - show/hide unfiltered lines");
}
