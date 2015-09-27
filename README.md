# README #

# Overview
Trv.js is a log file viewing program which utilizes JavaScript to enable powerful filtering options. Log file in this case is a text file which contains a set of records; every record written in a separate line. 

At a basic level, trv.js allows you to quickly load a file into memory, highlight some lines with colors using pattern matching and switch between displaying all lines or highlighted lines only. The size of the file is limited by amount of memory. For example to highlight all lines with word "error" in them with Red color type following in trv.js command line

    .a error Red

You can switch between displaying highlighted lines by running .sf command in the command line or using "ctrl+h" shortcut



# Building
To build trv.js you need VS2015. You also have to build V8 engine and set V8 environment variable to point to your copy of V8.
