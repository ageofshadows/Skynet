# encoding: UTF-8

"""
套利交易策略

注意事项：
1. 作者不对交易盈利做任何保证，策略代码仅供参考

"""


from vnpy.trader.app.ctaSpreadStrategy.arbBase import *
from vnpy.trader.app.ctaSpreadStrategy.arbTemplate import *
from vnpy.trader.vtObject import VtBarData, VtTickData

import json
import os
from binascii import a2b_hex,b2a_hex

import talib
import numpy as np

TICK_STOP = 0
TICK_CON1 = 1
TICK_CON2 = 2
TICK_CON3 = 3
TICK_CON4 = 4
TICK_SEND = 5

DIR_LONG  = u'LONG'
DIR_SHORT = u'SHORT'
DIR_NONE  = u'NONE'

OFF_OPEN  = u'OPEN'
OFF_CLOSE = u'CLOSE'
OFF_NONE  = u'NONE'

########################################################################
class ArbStrategy(ArbTemplate):
    """组合合约套利策略"""
    className = 'ArbStrategy'
    author = u'Being Dan'

    # 策略参数
    X1 = 1                  # 合约乘数1   
    C1 = u''                # 合约名1
    X2 = 1                  # 合约乘数2
    C2 = u''                # 合约名2
    X3 = 1                  # 合约乘数3
    C3 = u''                # 合约名3
    X4 = 1                  # 合约乘数4
    C4 = u''                # 合约名4
    volume =  0  	    # 单次开仓手数
    price =  0  	    # 单次开仓价位
    direction = u''  	    # 方向
    offset = u''  	    # 开平
    state = u'Untriggered'       # 状态

    # 策略变量
    tick1 = None               # 合约1 Tick
    tick2 = None               # 合约2 Tick
    tick3 = None               # 合约3 Tick
    tick4 = None               # 合约4 Tick

    price1 = EMPTY_FLOAT       # 合约1 成交价
    price2 = EMPTY_FLOAT       # 合约2 成交价
    price3 = EMPTY_FLOAT       # 合约3 成交价
    price4 = EMPTY_FLOAT       # 合约4 成交价

    askPrice1 = EMPTY_FLOAT    # 当前对价可买价
    bidPrice1 = EMPTY_FLOAT    # 当前对价可卖价

    contractF = u''            # 组合公式表示
    ncon = 0                   # 操作合约数
    symbolList = []            # 所有操作合约

    aPos =  0                  # 总持仓数
    aCost = 0                  # 平均持仓成本

    lastVolume = 0             # 上次发单数
    volumeRemain = 0           # 剩余订单

    Lock = False	       # 发单操作锁,防止重复发单
    refreshCost = False	       # 刷新成本,防止重复
    tickState = TICK_STOP      # 状态机状态


    # 参数列表，保存了参数的名称
    paramList = ['name',
                 'contractF',
                 'direction',
                 'offset',
                 'price',
                 'volume',
                 'state']

    # 变量列表，保存了变量的名称
    varList = ['inited',
               'trading',
               'aPos',
               'aCost',
               'tickState']

    #----------------------------------------------------------------------
    def __init__(self, arbEngine, setting):
        """Constructor"""
        super(ArbStrategy, self).__init__(arbEngine, setting)

        # 初始化所有变量
        self.tick1 = VtTickData()      # 合约1 Tick
        self.tick2 = VtTickData()      # 合约2 Tick
        self.tick3 = VtTickData()      # 合约3 Tick
        self.tick4 = VtTickData()      # 合约4 Tick

        self.price1 = EMPTY_FLOAT       # 合约1 成交价
        self.price2 = EMPTY_FLOAT       # 合约2 成交价
        self.price3 = EMPTY_FLOAT       # 合约3 成交价
        self.price4 = EMPTY_FLOAT       # 合约4 成交价


        self.askPrice1 = EMPTY_FLOAT    # 当前对价可买价
        self.bidPrice1 = EMPTY_FLOAT    # 当前对价可卖价

        self.contractF = u''            # 组合公式表示
        self.ncon = 0                   # 操作合约数
        self.symbolList = []            # 所有操作合约

        self.aPos =  0                  # 总持仓数
        self.aCost = 0                  # 平均持仓成本

        self.lastVolume = 0             # 上次发单数
        self.volumeRemain = 0           # 剩余订单
        
        self.lastVolumeC1 = 0             # C1 上次发单数

        self.Lock = False	        # 发单操作锁,防止重复发单
        self.refreshCost = False	       # 刷新成本,防止重复
        self.tickState = TICK_STOP      # 状态机状态

        self.productClass = PRODUCT_FOREX    # 产品类型（只有IB接口需要）
        self.currency = EMPTY_STRING        # 货币（只有IB接口需要）

        # 注意策略类中的可变对象属性（通常是list和dict等），在策略初始化时需要重新创建，
        # 否则会出现多个策略实例之间数据共享的情况，有可能导致潜在的策略逻辑错误风险，
        # 策略类中的这些可变对象属性可以选择不写，全都放在__init__下面，写主要是为了阅读
        # 策略时方便（更多是个编程习惯的选择）        

    #----------------------------------------------------------------------
    def onInit(self):
        """初始化策略（必须由用户继承实现）"""
        self.writeArbLog(u'%s策略初始化' %self.name)
        self.putEvent()

    #----------------------------------------------------------------------
    def onStart(self):
        """启动策略（必须由用户继承实现）"""
        self.writeArbLog(u'%s策略启动' %self.name)
        self.putEvent()

    #----------------------------------------------------------------------
    def onSend(self, data):
        """启动策略（必须由用户继承实现）"""
        if not self.tickState == TICK_STOP:
            self.writeArbLog(u'%s:请先等待上一笔订单完成' %self.name)
        else:
            self.price = float(data['price'])
            self.volume = int(data['volume'])
            self.direction = str(data['direction'])
            self.offset = str(data['offset'])
            self.tickState = TICK_SEND
            self.state = u'Triggered'
        self.putEvent()

    #----------------------------------------------------------------------
    def onRedraw(self):
        """启动策略（必须由用户继承实现）"""
        #if not self.tickState == TICK_SEND:
        #   self.writeArbLog(u'%s:上一笔订单已经触发' %self.name)
        #else:
        self.price = 0
        self.volume = 0
        self.tickState = TICK_STOP
        self.state = u'Untriggered'
        self.putEvent()

    #----------------------------------------------------------------------
    def onCalcPos(self, posDict):
        """启动策略（必须由用户继承实现）"""
        #if not self.tickState == TICK_SEND:
        #   self.writeArbLog(u'%s:上一笔订单已经触发' %self.name)
        #else:
        xList = [self.X1, self.X2, self.X3, self.X4]
        cList = [self.C1, self.C2, self.C3, self.C4]
        nList = [0, 0, 0, 0]
        b_plus = None
        lastn = 0
        for i in xrange(0,self.ncon):
            if cList[i] in posDict:
                if not cList[i] in self.allContracts:
                    self.allContracts[cList[i]] = arbContract(cList[i])
                    
                if posDict[cList[i]].direction == DIRECTION_LONG:
                    self.allContracts[cList[i]].tpos0L = posDict[cList[i]].position -  posDict[cList[i]].ydPosition
                    self.allContracts[cList[i]].ypos0L = posDict[cList[i]].ydPosition
                    n = int(posDict[cList[i]].position/xList[i])
                elif posDict[cList[i]].direction == DIRECTION_SHORT:
                    self.allContracts[cList[i]].tpos0S = posDict[cList[i]].position -  posDict[cList[i]].ydPosition
                    self.allContracts[cList[i]].ypos0S = posDict[cList[i]].ydPosition
                    n = int(-posDict[cList[i]].position/xList[i])
                    
                if lastn >=0 and n >= 0:
                    self.aPos = min(lastn, n)
                elif lastn <=0 and n<=0:
                    self.aPos = max(lastn, n)
                else:
                    self.aPos = 0
                    
                lastn = n
        
        self.putEvent()

    #----------------------------------------------------------------------
    def onUpdate(self,data):
        """刷新策略组合参数"""
        self.writeArbLog(u'%s策略组合刷新' %self.name)
        if not self.tickState == TICK_STOP:
            self.writeArbLog(u'请先停止策略再刷新')
            return
        if not self.pos == 0:
            self.writeArbLog(u'请先处理所有仓位再刷新')
            return
        xList = [data['X1'],data['X2'],data['X3'],data['X4']]
        cList = [data['C1'],data['C2'],data['C3'],data['C4']]
        d = self.__dict__
        self.symbolList = []
        self.ncon = 0
        self.contractF = '' 
        for i in xrange(1,5):
            d['X'+str(i)] = None
            d['C'+str(i)] = None
        for i in xrange(1,5):
            if not xList[i-1] == '':
                d['X'+str(i)] = int(xList[i-1])
                d['C'+str(i)] = str(cList[i-1])
                self.symbolList.append(str(cList[i-1]))
                self.ncon += 1
                if i == 1 or int(xList[i-1])<0:
                    self.contractF += str(xList[i-1])+'*'+str(cList[i-1])
                else:
                    self.contractF += '+'+str(xList[i-1])+'*'+str(cList[i-1])
            else:
                break

        self.arbEngine.subscribe(self, self.symbolList)
        self.putEvent()

    #----------------------------------------------------------------------
    def onStop(self):
        """停止策略（必须由用户继承实现）"""
        self.writeArbLog(u'%s策略停止' %self.name)
        self.putEvent()        

    #----------------------------------------------------------------------
    def onTick(self, tick):
        """收到行情TICK推送（必须由用户继承实现）"""

        if tick.symbol == self.C1:
            self.tick1 = tick
        elif tick.symbol == self.C2:
            self.tick2 = tick
        elif tick.symbol == self.C3:
            self.tick3 = tick
        elif tick.symbol == self.C4:
            self.tick4 = tick

        self.askPrice1 = 0
        self.bidPrice1 = 0

        d = self.__dict__
        allTicks = [None, self.tick1, self.tick2, self.tick3, self.tick4]

        for i in xrange(1,self.ncon+1):
            if d['X'+str(i)] > 0 and not allTicks[i]==None:
                self.askPrice1 += d['X'+str(i)]*allTicks[i].askPrice1
                self.bidPrice1 += d['X'+str(i)]*allTicks[i].bidPrice1
            elif d['X'+str(i)] < 0 and not allTicks[i]==None:
                self.askPrice1 += d['X'+str(i)]*allTicks[i].bidPrice1
                self.bidPrice1 += d['X'+str(i)]*allTicks[i].askPrice1
            else:
                break
        
        contract = None
        
        if not self.Lock and self.tickState == TICK_SEND: 
            if self.price > self.askPrice1:
                self.direction == DIR_LONG
                contract = self.allContracts[self.C1]
                if not contract:
                    self.offset == OFF_OPEN
                    self.reFillOrder(1,self.volume)
                elif contract.pos<0:
                    self.offset == OFF_CLOSE
                    self.reFillOrder(1,abs(contract.pos))
                elif contract.pos==0:
                    self.offset == OFF_OPEN
                    self.reFillOrder(1,self.volume)
            elif self.price < self.bidPrice1:
                self.direction == DIR_SHORT
                contract = self.allContracts[self.C1]
                if not contract:
                    self.offset == OFF_OPEN
                    self.reFillOrder(1,self.volume)
                elif contract.pos>0:
                    self.offset == OFF_CLOSE
                    self.reFillOrder(1,abs(contract.pos))
                elif contract.pos==0:
                    self.offset == OFF_OPEN
                    self.reFillOrder(1,self.volume)                
                    
        self.putEvent()

    #----------------------------------------------------------------------
    def onBar(self, bar):
        """收到Bar推送（必须由用户继承实现）"""

        # 保存K线数据
        self.closeArray[0:self.bufferSize-1] = self.closeArray[1:self.bufferSize]
        self.highArray[0:self.bufferSize-1] = self.highArray[1:self.bufferSize]
        self.lowArray[0:self.bufferSize-1] = self.lowArray[1:self.bufferSize]

        self.closeArray[-1] = bar.close
        self.highArray[-1] = bar.high
        self.lowArray[-1] = bar.low

        self.bufferCount += 1
        if self.bufferCount < self.bufferSize:
            return

    #----------------------------------------------------------------------
    def onOrder(self, order):
        """收到委托变化推送（必须由用户继承实现）"""
        # 对于无需做细粒度委托控制的策略，可以忽略onOrder
        pass        

    #----------------------------------------------------------------------
    def onOrderTrade(self, order):
        """Previous leg fully filled; order next contract"""
        # CTA委托类型映射
        if self.tickState == TICK_CON1:
            if self.ncon > 1:
                self.reFillOrder(2,self.volume)
            elif self.ncon <= 1:
                self.tickState = TICK_SEND
        elif self.tickState == TICK_CON2:
            self.tickState = TICK_SEND

        self.volumeRemain = 0
        self.Lock = False

    #----------------------------------------------------------------------
    def onOrderCancel(self, order):
        """收到委托完成推送（必须由用户继承实现）"""
        # CTA委托类型映射

        self.Lock = False
        volumeRemain = order.totalVolume - order.tradedVolume

        if self.tickState == TICK_CON1:
            if self.ncon > 1:
                self.reFillOrder(2,order.tradedVolume)
            elif self.ncon <= 1:
                self.tickState = TICK_SEND
        elif self.tickState == TICK_CON2:
            self.reFillOrder(2,volumeRemain)

    #----------------------------------------------------------------------
    def onTrade(self, trade):
        """收到成交推送（必须由用户继承实现）"""
        # 对于无需做细粒度委托控制的策略，可以忽略onTrade        
        super(ArbStrategy, self).onTrade(trade)
        
        contract = self.allContracts[trade.symbol]
        
        price = trade.price
        if trade.symbol == self.C1:
            self.price1 = trade.price
            self.lastVolume = abs(contract.pos)
            self.lastVolumeC1 = trade.volume
        elif trade.symbol == self.C2:
            self.price2 = trade.price
        elif trade.symbol == self.C3:
            self.price3 = trade.price
        elif trade.symbol == self.C4:
            self.price4 = trade.price

        volumeRemain = self.lastVolume - abs(contract.pos)  
            
        if volumeRemain == 0:
            self.onTradeFullFilled(self.lastVolumeC1)
        elif volumeRemain != 0:
            self.onTradePartFilled(abs(volumeRemain))
                    
    #----------------------------------------------------------------------
    def onTradeFullFilled(self, lastVolume):
        """Previous leg fully filled; order next contract"""
        # CTA委托类型映射
        if self.tickState == TICK_CON1:
            if self.ncon > 1:
                self.reFillOrder(2,lastVolume)
            elif self.ncon <= 1:
                self.tickState = TICK_SEND
        elif self.tickState == TICK_CON2:
            self.tickState = TICK_SEND

        self.volumeRemain = 0
        self.Lock = False
        
    #----------------------------------------------------------------------
    def onTradePartFilled(self, volumeRemain):
        """收到委托完成推送（必须由用户继承实现）"""
        # CTA委托类型映射

        self.Lock = False

        if self.tickState == TICK_CON2:
            self.reFillOrder(2,volumeRemain)

    #----------------------------------------------------------------------
    def reFillOrder(self, i, volumeRemain):

        askList = [self.tick1.askPrice1, self.tick2.askPrice1, self.tick3.askPrice1, self.tick4.askPrice1]
        bidList = [self.tick1.bidPrice1, self.tick2.bidPrice1, self.tick3.bidPrice1, self.tick4.bidPrice1]

        stateList = [TICK_CON1, TICK_CON2, TICK_CON3, TICK_CON4] 

        X = self.__dict__['X'+str(i)]
        C = self.__dict__['C'+str(i)]

        ask = askList[i-1]
        bid = bidList[i-1]

        state = stateList[i-1]

        if self.direction == DIR_LONG:
            if X > 0:
                self.buy(C,ask,volumeRemain*X)
            elif X < 0:
                self.short(C,bid,-volumeRemain*X)
        elif self.direction == DIR_SHORT:
            if X > 0:
                self.short(C,bid,volumeRemain*X)
            elif X < 0:
                self.buy(C,ask,-volumeRemain*X)

        self.Lock = True
        self.tickState = state

    #----------------------------------------------------------------------
    def calcCost(self):
        priceList = [self.price1, self.price2, self.price3, self.price4]
        xList = [self.X1, self.X2, self.X3, self.X4]
        cost = 0
        for i in xrange(0,self.ncon):
            cost += xList[i]*priceList[i]
        return cost

    #----------------------------------------------------------------------
    def output(self, content):
        """输出信息（必须由用户继承实现）"""
        # 输出信息
        if self.backtesting:
            self.arbEngine.output(content)

        self.writeArbLog(content)

    #----------------------------------------------------------------------
    def report(self):
        """报告策略状态"""
        pass

