# README #

# Overview
Trv.js is a log file viewing program which utilizes JavaScript to enable powerful filtering options. Log file in this case is a text file which contains a set of records; every record written in a separate line. 

At a basic level, trv.js allows you to quickly load a file into memory, highlight some lines with colors using pattern matching and switch between displaying all lines or highlighted lines only. The size of the file is limited by amount of memory. For example to highlight all lines with word "error" in them with Red color type following in trv.js command line

    .a error Red

You can switch between displaying highlighted lines by running .sf command in the command line or using "ctrl+h" shortcut

The functionality provided by trv.js does not end with simple highlighting. You can use JavaScript to perform complex processing of log files. Even ".a" command itself is implemented in JavaScript using set of core objects exposed in JavaScript. The whole source of .a as well as other commands is located in trv.std.js and looks as collowing

First we define "af" helper function which takes collection, color and title and adds a filter to tagger object. Tagger object manages highlight related functionalty. addFilter method returns a new filter which we store in $f variable

    function af(coll, color, title) {
        $f = $.tagger.addFilter(coll, color, title);
        return $f;
    }

Second we define a function "a" which creates takes an expression, color and title and finds all lines in a trace file which matches the expression. Passes the result collection as well as other parameters to af function

    function a(expression, color, title) {
        af($.trace.where(expression), color, title);
    }
    
You can call a("server", Red) in the command window to highlight the lines but this is not convenient. To reduce amount of typing trv.js supports dot expression syntax where function parameters can be specified without \" or commas. 

    $.dotexpressions.add("a", a);

To reduce typing even further, we are going to bind "ctrl+a" shortcut to move focus to command line and automatically enter ".a "

    function startEditFilter() {
        $.console.setFocus();
        $.console.setText(".a ");
    }
    $.shortcuts.add("ctrl+a", startEditFilter);

trv.std.js defines a set of basic methods and key bindings. trv.csi.js builds on top of functionality provided by trv.std.js and provides additional support for a particular type of log files.  

# Installation
Unzip trv.zip to a directory of your choice. Optionally create .user.js file either in this directory or under AppData\Local\trv.js for storing custom functionality. See example.user.js as an example. 

# Building
To build trv.js you need VS2015. You also have to build V8 engine and set V8 environment variable to point to your copy of V8.
