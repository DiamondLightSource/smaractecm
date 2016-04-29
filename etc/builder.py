from iocbuilder import Device, AutoSubstitution
from iocbuilder.modules.motor import MotorLib
from iocbuilder.modules.asynaid import Asynaid
from iocbuilder.modules.asyn import Asyn, AsynIP, AsynPort
from iocbuilder.arginfo import makeArgInfo, Simple, Ident

__all__ = ['SmaractMcsController', 'SmaractMcsAxis', 'smaractMcsControllerTemplate']

class SmaractMcsController(AsynPort):
    '''Serial Smaract MCS controller'''
    Dependencies = (Asyn, MotorLib, Asynaid )
    LibFileList = ['smaract']
    DbdFileList = ['smaract']

    def __init__(self, name, asynPort, asynAddr, maxNoAxes, 
                 movePollPeriod, notMovePollPeriod):
        self.__super.__init__(name)
        self.name = name
        self.asynPort = asynPort
        self.asynAddr = asynAddr
        self.maxNoAxes = maxNoAxes
        self.movePollPeriod = movePollPeriod
        self.notMovePollPeriod = notMovePollPeriod
        self.controllerNum = 1 #hardcoded
        
    def Initialise(self):
        print 'mcsControllerConfig("%(name)s",' \
            ' %(controllerNum)d,' \
            ' "%(asynPort)s",' \
            ' %(asynAddr)d, %(maxNoAxes)d,' \
            ' %(movePollPeriod)d,' \
            ' %(notMovePollPeriod)d)'  % self.__dict__

    ArgInfo = makeArgInfo(
        __init__,
        name = Simple("Controller's name", str),
        asynPort = Ident("controller's asyn port", AsynIP),
        asynAddr = Simple("address", int),
        maxNoAxes = Simple("max number of axes", int),
        movePollPeriod = Simple("poll period (ms) when moving", int),
        notMovePollPeriod = Simple("poll period (ms) when not moving", int)
    )

class smaractMcsControllerTemplate(AutoSubstitution):
    TemplateFile = 'smaractMcsController.template'
smaractMcsControllerTemplate.ArgInfo.descriptions["PORT"] = Ident(
    "Asyn port for record", SmaractMcsController)

class SmaractMcsAxis(AutoSubstitution, Device):
    '''Axis in smaract controller'''
    TemplateFile = 'smaractMcsAxis.template'
    def __init__(self, name, controller, axis_number, rotary, p, r, timeout, guilabel):
        self.__super.__init__( P=p, R=r, PORT=controller, AXIS=axis_number, TIMEOUT=timeout, name=name, label=guilabel)
        self.name = name
        self.controller = controller
        self.axis_number = axis_number
        self.rotary = rotary
    ArgInfo = makeArgInfo(
        __init__,
        name = Simple("axis name", str),
        controller = Ident("controller port name", SmaractMcsController),
        axis_number = Simple("axis number", int),
        rotary = Simple("rotary stage (yes/no)", bool),
        p = Simple("prefix for pvs in template", str),
        r = Simple("suffix for pvs in template", str),
        guilabel = Simple("Label for gui box", str),
        timeout = Simple("timeout for asyn", int)
    )
    def Initialise(self):
        print 'mcsAxisConfig("%(controller)s",' \
            ' %(axis_number)d,' \
            ' %(rotary)d)'  % self.__dict__

