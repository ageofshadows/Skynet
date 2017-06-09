# encoding: UTF-8

'''
ARB模块相关的GUI控制组件
'''


#from eventEngine import *

from vnpy.event import Event
from vnpy.trader.vtEvent import *
from vnpy.trader.uiBasicWidget import QtGui, QtCore, QtWidgets, BasicCell



########################################################################
class ArbValueMonitor(QtWidgets.QTableWidget):
    """参数监控"""

    #----------------------------------------------------------------------
    def __init__(self, parent=None):
        """Constructor"""
        super(ArbValueMonitor, self).__init__(parent)
        
        self.keyCellDict = {}
        self.data = None
        self.inited = False
        
        self.initUi()
        
    #----------------------------------------------------------------------
    def initUi(self):
        """初始化界面"""
        self.setRowCount(1)
        self.verticalHeader().setVisible(False)
        self.setEditTriggers(self.NoEditTriggers)
        
        self.setMaximumHeight(self.sizeHint().height())
        
    #----------------------------------------------------------------------
    def updateData(self, data):
        """更新数据"""
        dataKeys = []
        for key in data.keys():
            if key == 'inited':
		dataKeys.append(u'Initialized')
	    elif key == 'trading':
		dataKeys.append(u'Trading')
	    elif key == 'aPos':
		dataKeys.append(u'Total Position')
	    elif key == 'aCost':
		dataKeys.append(u'Average Cost')	    
            elif key == 'name':
                dataKeys.append(u'Strategy')
            elif key == 'contractF':
                dataKeys.append(u'Spread Formula')
            elif key == 'price':
                dataKeys.append(u'Spread')
            elif key == 'volume':
                dataKeys.append(u'Quantity')
            elif key == 'direction':
                dataKeys.append(u'Direction')
            elif key == 'offset':
                dataKeys.append(u'Offset')
            elif key == 'state':
                dataKeys.append(u'Order Status')
            else:
                dataKeys.append(key)

        if not self.inited:
            self.setColumnCount(len(data))
            self.setHorizontalHeaderLabels(dataKeys)
            col = 0
            for k, v in data.items():
                cell = QtWidgets.QTableWidgetItem(unicode(v))
                self.keyCellDict[k] = cell
                self.setItem(0, col, cell)
                col += 1
            
            self.inited = True
        else:
            for k, v in data.items():
                cell = self.keyCellDict[k]
                cell.setText(unicode(v))
        self.resizeColumnsToContents() 
        self.resizeRowsToContents() 


########################################################################
class ArbStrategyManager(QtWidgets.QGroupBox):
    """策略管理组件"""
    signal = QtCore.pyqtSignal(type(Event()))

    #----------------------------------------------------------------------
    def __init__(self, arbEngine, eventEngine, name, parent=None):
        """Constructor"""
        super(ArbStrategyManager, self).__init__(parent)
        
        self.arbEngine = arbEngine
        self.eventEngine = eventEngine
        self.name = name
        
        self.initUi()
        self.updateMonitor()
        self.registerEvent()
        
    #----------------------------------------------------------------------
    def initUi(self):
        """初始化界面"""
	self.setTitle(self.name)

        self.directionList = [u'LONG',
                              u'SHORT',
	                      u'NONE']

        self.offsetList = [u'OPEN',
                           u'CLOSE']

        labelX1 = QtWidgets.QLabel(u'Multiplier 1')
        labelS1 = QtWidgets.QLabel(u'   x')
        labelC1 =  QtWidgets.QLabel(u'Contract 1')
        self.lineX1 = QtWidgets.QLineEdit()
        self.lineS1 = QtWidgets.QLabel(u'   x')
        self.lineC1 = QtWidgets.QLineEdit()

        labelP2 = QtWidgets.QLabel(u'   +')
        labelX2 = QtWidgets.QLabel(u'Multiplier 2')
        labelS2 = QtWidgets.QLabel(u'   x')
        labelC2 =  QtWidgets.QLabel(u'Contract 2')
        self.lineP2 = QtWidgets.QLabel(u'   +')
        self.lineX2 = QtWidgets.QLineEdit()
        self.lineS2 = QtWidgets.QLabel(u'   x')
        self.lineC2 = QtWidgets.QLineEdit()

        labelP3 = QtWidgets.QLabel(u'   +')
        labelX3 = QtWidgets.QLabel(u'Multiplier 3')
        labelS3 = QtWidgets.QLabel(u'   x')
        labelC3 =  QtWidgets.QLabel(u'Contract 3')
        self.lineP3 = QtWidgets.QLabel(u'   +')
        self.lineX3 = QtWidgets.QLineEdit()
        self.lineS3 = QtWidgets.QLabel(u'   x')
        self.lineC3 = QtWidgets.QLineEdit()

        labelP4 = QtWidgets.QLabel(u'   +')
        labelX4 = QtWidgets.QLabel(u'Multiplier 4')
        labelS4 = QtWidgets.QLabel(u'   x')
        labelC4 =  QtWidgets.QLabel(u'Contract 4')
        self.lineP4 = QtWidgets.QLabel(u'   +')
        self.lineX4 = QtWidgets.QLineEdit()
        self.lineS4 = QtWidgets.QLabel(u'   x')
        self.lineC4 = QtWidgets.QLineEdit()

        labelX5 = QtWidgets.QLabel(u'Spread')
        labelC5 =  QtWidgets.QLabel(u'Quantity')
        self.lineX5 = QtWidgets.QLineEdit()
        self.lineC5 = QtWidgets.QLineEdit()

        labelX7 = QtWidgets.QLabel(u'Bought at')
        labelC7 =  QtWidgets.QLabel(u'Sold at')
        self.lineX7 = QtWidgets.QLabel(u'3349')
        self.lineC7 = QtWidgets.QLabel(u'3312')

        labelX6 =  QtWidgets.QLabel(u'Direction')
        self.comboDirection = QtWidgets.QComboBox()
        self.comboDirection.addItems(self.directionList)

        labelC6 =  QtWidgets.QLabel(u'Offset')
        self.comboOffset = QtWidgets.QComboBox()
        self.comboOffset.addItems(self.offsetList)

        # 代码输入框
        gridup = QtWidgets.QGridLayout()
        gridup.addWidget(labelX1, 0, 0)
        gridup.addWidget(labelS1, 0, 1)
        gridup.addWidget(labelC1, 0, 2)
        gridup.addWidget(labelP2, 0, 3)
        gridup.addWidget(labelX2, 0, 4)
        gridup.addWidget(labelS2, 0, 5)
        gridup.addWidget(labelC2, 0, 6)
        gridup.addWidget(labelP3, 0, 7)
        gridup.addWidget(labelX3, 0, 8)
        gridup.addWidget(labelS3, 0, 9)
        gridup.addWidget(labelC3, 0, 10)
        gridup.addWidget(labelP4, 0, 11)
        gridup.addWidget(labelX4, 0, 12)
        gridup.addWidget(labelS4, 0, 13)
        gridup.addWidget(labelC4, 0, 14)
        gridup.addWidget(self.lineX1, 1, 0)
        gridup.addWidget(self.lineS1, 1, 1)
        gridup.addWidget(self.lineC1, 1, 2)
        gridup.addWidget(self.lineP2, 1, 3)
        gridup.addWidget(self.lineX2, 1, 4)
        gridup.addWidget(self.lineS2, 1, 5)
        gridup.addWidget(self.lineC2, 1, 6)
        gridup.addWidget(self.lineP3, 1, 7)
        gridup.addWidget(self.lineX3, 1, 8)
        gridup.addWidget(self.lineS3, 1, 9)
        gridup.addWidget(self.lineC3, 1, 10)
        gridup.addWidget(self.lineP4, 1, 11)
        gridup.addWidget(self.lineX4, 1, 12)
        gridup.addWidget(self.lineS4, 1, 13)
        gridup.addWidget(self.lineC4, 1, 14)
        gridup.setColumnStretch(0, 1)
        gridup.setColumnStretch(1, 1)
        gridup.setColumnStretch(2, 2)
        gridup.setColumnStretch(3, 1)
        gridup.setColumnStretch(4, 1)
        gridup.setColumnStretch(5, 1)
        gridup.setColumnStretch(6, 2)
        gridup.setColumnStretch(7, 1)
        gridup.setColumnStretch(8, 1)
        gridup.setColumnStretch(9, 1)
        gridup.setColumnStretch(10, 2)
        gridup.setColumnStretch(11, 1)
        gridup.setColumnStretch(12, 1)
        gridup.setColumnStretch(13, 1)
        gridup.setColumnStretch(14, 2)
        gridup.setContentsMargins(2,2,600,10)

        griddown = QtWidgets.QGridLayout()
        griddown.addWidget(labelX7, 0, 0)
        griddown.addWidget(labelC7, 0, 1)
        griddown.addWidget(labelX5, 0, 2)
        griddown.addWidget(labelC5, 0, 3)
        griddown.addWidget(labelX6, 0, 4)
        griddown.addWidget(labelC6, 0, 5)
        griddown.addWidget(self.lineX7, 1, 0)
        griddown.addWidget(self.lineC7, 1, 1)
        griddown.addWidget(self.lineX5, 1, 2)
        griddown.addWidget(self.lineC5, 1, 3)
        griddown.addWidget(self.comboDirection, 1, 4)
        griddown.addWidget(self.comboOffset, 1, 5)
        griddown.setContentsMargins(2,2,1300,20)

        
        self.paramMonitor = ArbValueMonitor(self)
        self.varMonitor = ArbValueMonitor(self)
        
        maxHeight = 80
        #self.paramMonitor.setMaximumHeight(maxHeight)
        self.paramMonitor.setMinimumHeight(maxHeight)
        self.paramMonitor.resizeRowsToContents() 
        #self.varMonitor.setMaximumHeight(maxHeight)
        self.varMonitor.setMinimumHeight(maxHeight)
        self.varMonitor.resizeRowsToContents()
        
        buttonUpdate = QtWidgets.QPushButton(u'Update Contracts')
	buttonInit = QtWidgets.QPushButton(u'Initialize')
	buttonSend = QtWidgets.QPushButton(u'Trigger')
	buttonStart = QtWidgets.QPushButton(u'Start')	
        buttonClear = QtWidgets.QPushButton(u'Clear Strategy')
        buttonQrypos = QtWidgets.QPushButton(u'Query Position')
        buttonReport = QtWidgets.QPushButton(u'Report Strategy')
        buttonStop = QtWidgets.QPushButton(u'Stop')
        buttonUpdate.clicked.connect(self.update)
	buttonInit.clicked.connect(self.init)
	buttonSend.clicked.connect(self.send)
	buttonStart.clicked.connect(self.start)	
        buttonClear.clicked.connect(self.clear)
        buttonQrypos.clicked.connect(self.qrypos)
        buttonReport.clicked.connect(self.report)
        buttonStop.clicked.connect(self.stop)
        
        hbox1 = QtWidgets.QHBoxLayout()     
        hbox1.addWidget(buttonUpdate)
	hbox1.addWidget(buttonInit)
	hbox1.addWidget(buttonSend)
	hbox1.addWidget(buttonStart)	
        hbox1.addWidget(buttonClear)
        hbox1.addWidget(buttonQrypos)
        hbox1.addWidget(buttonReport)
        hbox1.addWidget(buttonStop)
        hbox1.addStretch()
        
        hbox2 = QtWidgets.QHBoxLayout()
        hbox2.addWidget(self.paramMonitor)
        
        hbox3 = QtWidgets.QHBoxLayout()
        hbox3.addWidget(self.varMonitor)
        
        vbox = QtWidgets.QVBoxLayout()
        vbox.addLayout(gridup)
        vbox.addLayout(griddown)
        vbox.addLayout(hbox1)
        vbox.addLayout(hbox2)
        vbox.addLayout(hbox3)

        vbox.setContentsMargins(20,50,20,20)

        self.setLayout(vbox)
        
    #----------------------------------------------------------------------
    def updateMonitor(self, event=None):
        """显示策略最新状态"""
        priceDict = self.arbEngine.getStrategyPrice(self.name)
        if priceDict:
            self.updatePrice(priceDict)

        paramDict = self.arbEngine.getStrategyParam(self.name)
        if paramDict:
            self.paramMonitor.updateData(paramDict)
            
        varDict = self.arbEngine.getStrategyVar(self.name)
        if varDict:
            self.varMonitor.updateData(varDict)        
            
    #----------------------------------------------------------------------
    def updatePrice(self, data):
        """更新价格"""
        self.lineX7.setText(str(data['askPrice1']))
        self.lineC7.setText(str(data['bidPrice1']))

    #----------------------------------------------------------------------
    def registerEvent(self):
        """注册事件监听"""
        self.signal.connect(self.updateMonitor)
        self.eventEngine.register(EVENT_CTA_STRATEGY+self.name, self.signal.emit)
    
    #----------------------------------------------------------------------
    def update(self):
        """初始化策略"""
        x1 = str(self.lineX1.text())
        c1 = str(self.lineC1.text())
        x2 = str(self.lineX2.text())
        c2 = str(self.lineC2.text())
        x3 = str(self.lineX3.text())
        c3 = str(self.lineC3.text())
        x4 = str(self.lineX4.text())
        c4 = str(self.lineC4.text())
        data = {'X1':x1, 'C1':c1, 'X2':x2, 'C2':c2, 'X3':x3, 'C3':c3, 'X4':x4, 'C4':c4}
        self.arbEngine.updateStrategy(self.name,data)
    
    #----------------------------------------------------------------------
    def init(self):
	"""初始化策略"""
	self.arbEngine.initStrategy(self.name)
    
    #----------------------------------------------------------------------
    def start(self):
        """启动策略"""
        self.arbEngine.startStrategy(self.name)
        
    #----------------------------------------------------------------------
    def send(self, data):
        """启动策略"""
        data = {'price':self.lineX5.text(), 'volume':self.lineC5.text(),
                'offset':self.comboOffset.currentText(),'direction':self.comboDirection.currentText()}
        self.arbEngine.sendStrategy(self.name, data)
        
    #----------------------------------------------------------------------
    def clear(self):
        """启动策略"""
        self.arbEngine.clearStrategy(self.name)
        
    #----------------------------------------------------------------------
    def cancel(self):
        """启动回测"""
        self.arbEngine.cancelStrategy(self.name)
        
    #----------------------------------------------------------------------
    def qrypos(self):
        """启动回测"""
        self.arbEngine.qryposStrategy(self.name)
        
    #----------------------------------------------------------------------
    def report(self):
        """启动报告"""
        self.arbEngine.reportStrategy(self.name)
        
    #----------------------------------------------------------------------
    def stop(self):
        """停止策略"""
        self.arbEngine.stopStrategy(self.name)


########################################################################
class ArbEngineManager(QtWidgets.QWidget):
    """ARB引擎管理组件"""
    signal = QtCore.pyqtSignal(type(Event()))

    #----------------------------------------------------------------------
    def __init__(self, arbEngine, eventEngine, parent=None):
        """Constructor"""
        super(ArbEngineManager, self).__init__(parent)
        
        self.arbEngine = arbEngine
        self.eventEngine = eventEngine
        
	self.nameList = []
	self.strategyManagers = {}
        self.strategyLoaded = False
        
        self.initUi()
        self.registerEvent()
        
        # 记录日志
        self.arbEngine.writeArbLog(u'Arbitrage engine initialized.')        
        
    #----------------------------------------------------------------------
    def initUi(self):
        """初始化界面"""
        self.setWindowTitle(u'CTA Strategy')
        
        # 按钮
        loadButton = QtWidgets.QPushButton(u'Load Strategy')
        updateMCButton = QtWidgets.QPushButton(u'Update Margin/Commission')
        
        loadButton.clicked.connect(self.load)
        updateMCButton.clicked.connect(self.updateMC)
        
        # 滚动区域，放置所有的ArbStrategyManager
        self.scrollArea = QtWidgets.QScrollArea()
        self.scrollArea.setWidgetResizable(True)
        
        # ARB组件的日志监控
        self.arbLogMonitor = QtWidgets.QTextEdit()
        self.arbLogMonitor.setReadOnly(True)
        self.arbLogMonitor.setMaximumHeight(200)
        
        # 设置布局
        hbox2 = QtWidgets.QHBoxLayout()
        hbox2.addWidget(loadButton)
        hbox2.addWidget(updateMCButton)
        hbox2.addStretch()
        
        vbox = QtWidgets.QVBoxLayout()
        vbox.addLayout(hbox2)
        vbox.addWidget(self.scrollArea)
        vbox.addWidget(self.arbLogMonitor)
        self.setLayout(vbox)
        
    #----------------------------------------------------------------------
    def initStrategyManager(self):
        """初始化策略管理组件界面"""        
        w = QtWidgets.QWidget()
	vbox = QtWidgets.QVBoxLayout()
        
        for name in sorted(self.arbEngine.strategyDict.keys()):
	    if name not in self.nameList:
		strategyManager = ArbStrategyManager(self.arbEngine, self.eventEngine, name)
		self.strategyManagers[name] = strategyManager
	    else:
		strategyManager = self.strategyManagers[name]
	    vbox.addWidget(strategyManager)
	    self.nameList.append(name)
        
        vbox.addStretch()
        
        w.setLayout(vbox)
        self.scrollArea.setWidget(w)   
        
    #----------------------------------------------------------------------
    def load(self):
        """加载策略"""
        if not self.strategyLoaded:
            self.arbEngine.loadSetting()
            self.initStrategyManager()
            self.strategyLoaded = True
            self.arbEngine.writeArbLog(u'Strategy loaded.')
	else:
            self.arbEngine.loadSetting()
            self.initStrategyManager()
            self.strategyLoaded = True
            self.arbEngine.writeArbLog(u'Strategy loaded.')
	    
    #----------------------------------------------------------------------
    def updateMC(self):
        """费率查询"""
        self.arbEngine.updateMC()
        
    #----------------------------------------------------------------------
    def updateArbLog(self, event):
        """更新ARB相关日志"""
        log = event.dict_['data']
        content = '\t'.join([log.logTime, log.logContent])
        self.arbLogMonitor.append(content)
    
    #----------------------------------------------------------------------
    def registerEvent(self):
        """注册事件监听"""
        self.signal.connect(self.updateArbLog)
        self.eventEngine.register(EVENT_CTA_LOG, self.signal.emit)
	
    #----------------------------------------------------------------------
    def closeEvent(self, event):
	"""关闭窗口时的事件"""
	pass
        
        
    
    
    
    



    
    
