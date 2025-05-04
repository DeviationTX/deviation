# This script will profile the qemu.bin build and produce a calltrace log
# It is meant to be loaded from within gdb via:
#   arm-none-eabi-gdb-py
#     source gdb
#     profile
# An STM32 QEMU is needed (either docker or natively compiled): https://github.com/beckus/qemu_stm32
# QEMU is started before running gdb via:
#   qemu-system-arm -S -s -M stm32-p103 -kernel qemu.bin

import gdb

compress = True

class func:
    def __init__(self, parent, pc, lr, name):
        self.exclusive = 0
        self.parent = parent
        self.pc = pc
        self.lr = lr
        self.children = []
        self.name = name
    def newchild(self, pc, lr, name):
        if compress:
            for child in self.children:
                if child.pc == pc:
                    child.lr = lr
                    return child
        newfunc = func(self, pc, lr, name)
        self.children.append(newfunc)
        return newfunc

def printstack(trace, depth=""):
    inclusive = trace.exclusive
    childstr = ""
    for child in trace.children:
        _str, incl = printstack(child, depth + "  ")
        childstr += _str
        inclusive += incl
    childstr =  "{}{} 0x{:02x} (ex:{} inc:{})\n".format(depth, trace.name, trace.pc, trace.exclusive, inclusive) + childstr
    if depth == "":
        print childstr
    else:
        return childstr, inclusive

topfunc = func(None, 0, 0, None)

class Profile(gdb.Command):
    def __init__(self):
        # This registers our class as "simple_command"
        super(Profile, self).__init__("profile", gdb.COMMAND_DATA)
        gdb.execute("file qemu.elf")
        gdb.execute("target remote localhost:1234")
        gdb.execute("set pagination off")
        self.trace = topfunc

    def invoke(self, arg, from_tty):
        # When we call "simple_command" from gdb, this is the method
        # that will be called.
        #gdb.execute("set logging file /dev/null")
        #gdb.execute("set logging redirect on")
        #gdb.execute("set logging on")
        gdb.execute("b run_profile")
        gdb.execute("c")
        gdb.execute("disable")
        stop_pc = gdb.newest_frame().older().pc()
   
        last_lr = int(gdb.parse_and_eval('$lr'))-1
        while True:
            frame = gdb.newest_frame()
            pc = frame.pc()
            lr = int(gdb.parse_and_eval('$lr'))-1
            if pc == self.trace.lr:
                print "Returned from {:02x}".format(self.trace.pc)
                self.trace = self.trace.parent
                #return
            elif lr != last_lr:
                self.trace = self.trace.newchild(pc, lr, frame.name())
                print "Called {}{:02x} (return: {:02x})".format(frame.name(), pc, lr)
                #return
            if pc == 0 or pc == stop_pc:
                break;
            self.trace.exclusive += 1
            last_lr = lr
            gdb.execute("si")
        #gdb.execute("set logging off")
        #gdb.execute("display")
        printstack(topfunc)

Profile()
