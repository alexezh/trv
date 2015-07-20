function help()
{
    $.print("Trv.js is a trace viewer which provides extensive filtering capabilities by utilizing jscript runtime.");
    $.print("For example running following command will cause lines which match filter be highlighter with Red color");
    $.print("  vsw(\"my line\", Red, \"comment\")");
    $.print("The list of current filters can be viewed by running");
    $.print("  vp()");
    $.print("Dot syntax");
    $.print("  Trv allows defining alternative syntax for a command by calling");
    $.print("     $.dotexpressions(\"foo\", foo)");
    $.print("  foo method can be invoked by");
    $.print("     .foo param1 param2");
    $.print("  parameters passed without commas or quotas");
    $.print("Keyboard");
    $.print("  Ctrl+H - show/hide unfiltered lines");
    $.print("  Ctrl+C - move focus to command line");
    $.print("Script files");
    $.print("  Trv loads script files by scanning directories: current, AppData\\Local\\trv.js, app binary");
    $.print("  trv.std.js   - contains standard functions");
    $.print("  .user.js     - contains user defined functions");
    $.print("  trv.help.js  - this help");
    $.print("  trv.test.js  - basic tests can be used as samples");
}
