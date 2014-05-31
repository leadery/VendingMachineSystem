VendingMachineSystem
====================
### program1: user side
    Before starting the program"
    usage: ./program1
    show instructions
    or: ./program1 test
    run self unit test
    or: ./program1 [AC ipv4 address] [AC port number] [local port number] start the program and use keyboard as input
    or: ./program1 [AC ipv4 address] [AC port number] [local port number] < [file ..] start the program and use file as input
    AC stands for authentication center.
    After starting the program, use <COMMAND> as input
    <COMMAND>:
    LOGIN <USERID> <PASSWORD>
    DEPOSIT $<DOLLARS>
    BALANCE
    BUY <INDEX>
    PRICE <INDEX>
    STATUS
    LOGOUT
    EXIT
    <USERID> is the user id
    <PASSWORD> is the user password
    <DOLLARS> is the money user wants to deposit
    <INDEX> is the item number(A followed by an integer, e.g. A1)
    Note: LOGIN command is used in LOGIN MODE. Other commands are used in user request mode.
### program2: authentication center
    usage: ./program2
    show instructions
    or: ./program2 test
    run unit self test
    or: ./program2 [local port number] [vending server ipv4 address] [vending ser
    ver port number]
### program3: vending server side
    Before starting the program:
    usage: ./program3 test
    run self unit test
    or: ./program3 [local port number] [user ipv4 address] [user port number] start the program and use keyboard as input
    or: ./program3 [local port number] [user ipv4 address] [user port number] < [file ..] start the program and use file as input
    After starting the program, use <COMMAND> as input
    In STARTUP MODE:
    ITEMS <NUMBER>
    <NUMBER> indicates the number of items to be loaded
    ITEM <XYZ> <DOLLARS>d
    <XYZ> is the name of the item(1−20 uppercase letters, no space)
    <DOLLARS> is the price prefixed by the $ sign
