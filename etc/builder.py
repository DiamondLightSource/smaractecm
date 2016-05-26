from iocbuilder import Device, AutoSubstitution
from iocbuilder.modules.motor import MotorLib
from iocbuilder.modules.asynaid import Asynaid
from iocbuilder.modules.asyn import Asyn, AsynIP, AsynPort
from iocbuilder.arginfo import makeArgInfo, Simple, Ident
from _hotshot import resolution

__all__ = ['SmaractEcmController', 'SmaractSmarpodAxis', 'Smarpod']


class _smaractEcmControllerTemplate(AutoSubstitution):
    TemplateFile = 'smaractEcmController.template'


# the following class should be as follows but it breaks the superclass init
# class SmaractEcmController(_smaractEcmControllerTemplate, AsynPort):
class SmaractEcmController(_smaractEcmControllerTemplate, Device):
    '''Smaract ECM controller'''
    Dependencies = (Asyn, MotorLib, Asynaid)
    LibFileList = ['smaractEcm']
    DbdFileList = ['smaractEcm']
    UniqueName = 'PORT'

    # this string conversion is a substitute for deriving from Asyn
    # We can't derive from Asyn because the init functions for Asyn
    # and Autosubstitutuion have incompatible init functions
    # but the below is the only feature we need from Asyn (deliver 
    # your port name when converting to string - instead of class name) 
    def __str__(self):
        return self.PORT
    
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
    def __init__(self, name, controller, resolution, unit):
        self.__super.__init__()
        self.__dict__.update(locals())
        print "creating smarpod %s with controller %s, res %s" % \
            (name, controller, self.resolution)

    ArgInfo = makeArgInfo(
        __init__,
        name=Simple("Smarpod name", str),
        controller=Ident("Ecm controller port name", SmaractEcmController),
        resolution=Simple("Smarpod axes resolution in m" \
                          " (defines smallest possible step)", float),
        unit=Simple("Smarpod's unit number on the ECM", int)
    )
    def Initialise(self):
        print 'smarpodConfig("%(name)s",' \
            ' "%(controller)s", ' \
            '"%(resolution)s",' \
            '"%(unit)d")' % self.__dict__

class _smaractSmarpodAxisTemplate(AutoSubstitution):
    TemplateFile = 'smarpodAxis.template'


class SmaractSmarpodAxis(_smaractSmarpodAxisTemplate, Device):
    '''Smarpod axis in smaract controller'''
    TemplateFile = 'smarpodAxis.template'
    def __init__(self, name, controller, axis_number, smarpod, smarpodAxis, p, r, timeout):
        self.__super.__init__(P=p, R=r, PORT=controller,
                              AXIS=axis_number, TIMEOUT=timeout)
        self.__dict__.update(locals())

    ArgInfo = makeArgInfo(
        __init__,
        name=Simple("axis name", str),
        controller=Ident("controller port name", SmaractEcmController),
        axis_number=Simple("unique axis number for controller (match basic_asyn_motor axis no.)", int),
        smarpod=Simple("smarpod name", str),
        smarpodAxis=Simple("smarpod axis (0=x 1=y 2=z 3=rx 4=ty 5=rz)", int),
        p=Simple("prefix for pvs in template", str),
        r=Simple("suffix for pvs in template", str),
        timeout=Simple("timeout for asyn", int)
    )
    def Initialise(self):
        print 'smarpodAxisConfig("%(controller)s",' \
            ' %(axis_number)d,' \
            ' %(smarpod)s,' \
            ' %(smarpodAxis)s)' % self.__dict__
