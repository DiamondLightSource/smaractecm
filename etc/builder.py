from iocbuilder import Device, AutoSubstitution
from iocbuilder.modules.motor import MotorLib
from iocbuilder.modules.asynaid import Asynaid
from iocbuilder.modules.asyn import Asyn, AsynIP, AsynPort
from iocbuilder.arginfo import makeArgInfo, Simple, Ident

__all__ = ['SmaractEcmController', 'SmaractSmarpodAxis', 'Smarpod']

class _smaractEcmControllerTemplate(AutoSubstitution):
    TemplateFile = 'smaractEcmController.template'

# the following class should be as follows but it breaks the superclass init
# class SmaractEcmController(_smaractEcmControllerTemplate, AsynPort):
class SmaractEcmController(_smaractEcmControllerTemplate, Device):
    '''Smaract ECM controller'''
    Dependencies = (Asyn, MotorLib, Asynaid )
    LibFileList = ['smaractEcm']
    DbdFileList = ['smaractEcm']
    UniqueName = 'PORT'

    def __init__(self, asynPort, asynAddr, maxNoAxes,
                 movePollPeriod, notMovePollPeriod, **args):
        self.__super.__init__(**args)
        self.__dict__.update(**args)
        self.__dict__.update(locals())

    def Initialise(self):
        print 'ecmControllerConfig("%(PORT)s",' \
            ' "%(asynPort)s",' \
            ' %(asynAddr)d, %(maxNoAxes)d,' \
            ' %(movePollPeriod)d,' \
            ' %(notMovePollPeriod)d)' % self.__dict__

    ArgInfo = _smaractEcmControllerTemplate.ArgInfo + makeArgInfo(
        __init__,
        asynPort=Ident("controller's asyn port", AsynIP),
        asynAddr=Simple("address", int),
        maxNoAxes=Simple("max number of axes", int),
        movePollPeriod=Simple("poll period (ms) when moving", int),
        notMovePollPeriod=Simple("poll period (ms) when not moving", int)
    )

class Smarpod(Device):
    '''Smarpod in smaract Ecm controller'''
    def __init__(self, name, controller):
        self.name = name
        self.controller = controller
    ArgInfo = makeArgInfo(
        __init__,
        name=Simple("Smarpod name", str),
        controller=Ident("Ecm controller port name", SmaractEcmController)
    )
    def Initialise(self):
        print 'smarpodAxisConfig("%(controller)s",' \
            ' %(axis_number)d,' \
            ' %(rotary)d)' % self.__dict__

class _smaractSmarpodAxisTemplate(AutoSubstitution):
    TemplateFile = 'smarpodAxis.template'
    
class SmaractSmarpodAxis(_smaractSmarpodAxisTemplate, Device):
    '''Smarpod axis in smaract controller'''
    TemplateFile = 'smarpodAxis.template'
    def __init__(self, name, controller, axis_number, smarpod, p, r, timeout):
        self.__super.__init__(P=p, R=r, PORT=controller.PORT, 
                              AXIS=axis_number, TIMEOUT=timeout, name=name)
        self.__dict__.update(locals())

    ArgInfo = makeArgInfo(
        __init__,
        name=Simple("axis name", str),
        controller=Ident("controller port name", SmaractEcmController),
        axis_number=Simple("axis number", int),
        smarpod=Simple("smarpod name", str),
        p=Simple("prefix for pvs in template", str),
        r=Simple("suffix for pvs in template", str),
        timeout=Simple("timeout for asyn", int)
    )
    def Initialise(self):
        print 'smarpodAxisConfig("%(controller)s",' \
            ' %(axis_number)d,' \
            ' %(smarpod)s)' % self.__dict__
