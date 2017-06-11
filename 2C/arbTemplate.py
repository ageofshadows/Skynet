# encoding: UTF-8

'''
本文件包含了CTA引擎中的策略开发用模板，开发策略时需要继承ArbTemplate类。
'''
import datetime
import copy
from vnpy.trader.app.ctaSpreadStrategy.arbBase import *
from vnpy.trader.vtConstant import *
from vnpy.trader.vtGateway import *



########################################################################
class ArbTemplate(object):
    """CTA策略模板"""
    
    # 策略类的名称和作者
    className = 'ArbTemplate'
    author = EMPTY_UNICODE
    
    # MongoDB数据库的名称，K线数据库默认为1分钟
    tickDbName = TICK_DB_NAME
    barDbName = MINUTE_DB_NAME
    
    # 策略的基本参数
    name = EMPTY_UNICODE           # 策略实例名称
    vtSymbol = EMPTY_STRING        # 交易的合约vt系统代码
    productClass = EMPTY_STRING    # 产品类型（只有IB接口需要）
    currency = EMPTY_STRING        # 货币（只有IB接口需要）

    allContracts = {}              # 所有合约

    # 参数列表，保存了参数的名称
    paramList = ['name',
                 'className',
                 'author',
                 'vtSymbol']
    
    # 变量列表，保存了变量的名称
    varList = ['inited',
               'trading',
               'pos']

    #----------------------------------------------------------------------
    def __init__(self, arbEngine, setting):
        """Constructor"""
        self.arbEngine = arbEngine

        self.productClass = EMPTY_STRING    # 产品类型（只有IB接口需要）
        self.currency = EMPTY_STRING        # 货币（只有IB接口需

        # 策略的基本变量，由引擎管理
        self.inited = False                 # 是否进行了初始化
        self.trading = False                # 是否启动交易，由引擎管理
        self.backtesting = False            # 回测模式

        self.pos = 0			   # 总投机方向

        self.tpos0L = 0			   # 今持多仓
        self.tpos0S = 0			   # 今持空仓
        self.ypos0L = 0			   # 昨持多仓
        self.ypos0S = 0			   # 昨持空仓

        # 设置策略的参数
        if setting:
            d = self.__dict__
            for key in self.paramList:
                if key in setting:
                    d[key] = setting[key]

    #----------------------------------------------------------------------
    def onInit(self):
        """初始化策略（必须由用户继承实现）"""
        raise NotImplementedError
    
    #----------------------------------------------------------------------
    def onUpdate(self,setting):
        """启动策略（必须由用户继承实现）"""
        if setting:
            d = self.__dict__
            for key in self.paramList:
                if key in setting:
                    d[key] = setting[key]
    
    #----------------------------------------------------------------------
    def onStart(self):
        """启动策略（必须由用户继承实现）"""
        raise NotImplementedError
    
    #----------------------------------------------------------------------
    def onStop(self):
        """停止策略（必须由用户继承实现）"""
        raise NotImplementedError

    #----------------------------------------------------------------------
    def confSettle(self, vtSymbol):
        """确认结算信息"""
	self.arbEngine.confSettle(vtSymbol)

    #----------------------------------------------------------------------
    def onTick(self, tick):
	"""收到行情TICK推送（必须由用户继承实现）"""
	raise NotImplementedError

    #----------------------------------------------------------------------
    def onOrder(self, order):
        """收到委托变化推送（必须由用户继承实现）"""
	raise NotImplementedError

    #----------------------------------------------------------------------
    def onTrade(self, trade):
        """收到成交推送（必须由用户继承实现）"""
        # 对于无需做细粒度委托控制的策略，可以忽略onOrder
        # CTA委托类型映射
        contract = None
        vtSymbol = trade.symbol
	
        for name in self.allContracts.keys():
            if name == vtSymbol:
                contract = self.allContracts[name]
        if not contract:
            contract = arbContract(vtSymbol)
            self.allContracts[vtSymbol] = contract
	    
        if trade != None and trade.direction == DIRECTION_LONG:
	    contract.pos += trade.volume
        if trade != None and trade.direction == DIRECTION_SHORT:
	    contract.pos -= trade.volume
    
    #----------------------------------------------------------------------
    def onBar(self, bar):
        """收到Bar推送（必须由用户继承实现）"""
        raise NotImplementedError
    
    #----------------------------------------------------------------------
    def buy(self, vtSymbol, price, volume, stop=False):
        """买开"""
        return self.sendOrder(vtSymbol, CTAORDER_BUY, price, volume, stop)
    
    #----------------------------------------------------------------------
    def sell(self, vtSymbol, price, volume, stop=False):
	"""卖平"""
	return self.sendOrder(vtSymbol, CTAORDER_SELL, price, volume, stop)       

    #----------------------------------------------------------------------
    def short(self, vtSymbol, price, volume, stop=False):
	"""卖开"""
	return self.sendOrder(vtSymbol, CTAORDER_SHORT, price, volume, stop)          

    #----------------------------------------------------------------------
    def cover(self, vtSymbol, price, volume, stop=False):
	"""买平"""
	return self.sendOrder(vtSymbol, CTAORDER_COVER, price, volume, stop)              

    #----------------------------------------------------------------------
    def sendOrder(self, vtSymbol, orderType, price, volume, stop=False):
        """发送委托"""
        if self.trading:
            # 如果stop为True，则意味着发本地停止单
            if stop:
                vtOrderID = self.arbEngine.sendStopOrder(vtSymbol, orderType, price, volume, self)
            else:
                vtOrderID = self.arbEngine.sendOrder(vtSymbol, orderType, price, volume, self) 
            return vtOrderID
        else:
	    # 交易停止时发单返回空字符串
            return ''     

    #----------------------------------------------------------------------
    def cancelOrder(self, vtOrderID):
        """撤单"""
	# 如果发单号为空字符串，则不进行后续操作
	if not vtOrderID:
	    return
	
        return self.arbEngine.cancelOrder(vtOrderID)
        #if STOPORDERPREFIX in vtOrderID:
        #    return self.arbEngine.cancelStopOrder(vtOrderID)
        #else:
        #    return self.arbEngine.cancelOrder(vtOrderID)
    
    #----------------------------------------------------------------------
    def insertTick(self, tick):
        """向数据库中插入tick数据"""
        self.arbEngine.insertData(self.tickDbName, self.vtSymbol, tick)
    
    #----------------------------------------------------------------------
    def insertBar(self, bar):
        """向数据库中插入bar数据"""
        self.arbEngine.insertData(self.barDbName, self.vtSymbol, bar)
        
    #----------------------------------------------------------------------
    def loadTick(self, days):
        """读取tick数据"""
        return self.arbEngine.loadTick(self.tickDbName, self.vtSymbol, days)
    
    #----------------------------------------------------------------------
    def loadBar(self, days):
        """读取bar数据"""
        return self.arbEngine.loadBar(self.barDbName, self.vtSymbol, days)
    
    #----------------------------------------------------------------------
    def writeArbLog(self, content):
        """记录CTA日志"""
        content = self.name + ':' + content
        self.arbEngine.writeArbLog(content)
        
    #----------------------------------------------------------------------
    def putEvent(self):
        """发出策略状态变化事件"""
        self.arbEngine.putStrategyEvent(self.name)
	
    #----------------------------------------------------------------------
    def getEngineType(self):
	"""查询当前运行的环境"""
	return self.ctaEngine.engineType
    
########################################################################
class arbContract(object):
    """合约信息模板"""
    vtSymbol = ''                  # 合约名
    pos = 0			   # 总投机方向

    tpos0L = 0			   # 今持多仓
    tpos0S = 0			   # 今持空仓
    ypos0L = 0			   # 昨持多仓
    ypos0S = 0			   # 昨持空仓

    #----------------------------------------------------------------------
    def __init__(self,vtSymbol, pos=0, tpos0L=0, tpos0S=0, ypos0L=0, ypos0S=0):
        """Constructor"""
        
        self.vtSymbol = vtSymbol           # 合约名
        self.pos = pos			   # 总投机方向

        self.tpos0L = tpos0L	           # 今持多仓
        self.tpos0S = tpos0S	           # 今持空仓
        self.ypos0L = ypos0L	           # 昨持多仓
        self.ypos0S = ypos0S	           # 昨持空仓

    #----------------------------------------------------------------------
    def update(self, pos, tpos0L, tpos0S, ypos0L, ypos0S):
        """Constructor"""
        self.pos = pos			   # 总投机方向

        self.tpos0L = tpos0L	           # 今持多仓
        self.tpos0S = tpos0S	           # 今持空仓
        self.ypos0L = ypos0L	           # 昨持多仓
        self.ypos0S = ypos0S	           # 昨持空仓

    #----------------------------------------------------------------------
    def updateDay(self, pos, tpos0L, tpos0S, ypos0L, ypos0S):
        """Constructor"""
	self.ypos0L += self.tpos0L
	self.tpos0L = 0
	self.ypos0S += self.tpos0S
	self.tpos0S = 0

