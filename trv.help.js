function help() {
    $.print("Trv.js is a trace viewer which provides extensive filtering capabilities by utilizing JavaScript runtime.");
    $.print("Commands");
    for (i in commandHelp) {
        $.print(commandHelp[i].id + " " + commandHelp[i].desc);
    }
    $.print("\r\nTrv loads script files by scanning directories: current, AppData\\Local\\trv.js, app binary");
    $.print("  trv.std.js   - contains standard functions");
    $.print("  .user.js     - contains user defined functions");
    $.print("  trv.help.js  - this help");
    $.print("  trv.test.js  - basic tests can be used as samples");
}
