#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Weborf
# Copyright (C) 2010  Salvo "LtWorf" Tomaselli
# 
# Weborf is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
# author Salvo "LtWorf" Tomaselli <tiposchi@tiscali.it>

from PyQt4 import QtCore, QtGui
import main
import whelper
import nhelper
import sys
import os, os.path

class qweborfForm (QtGui.QWidget):
    
    DBG_DEFAULT=0
    DBG_WARNING=1
    DBG_ERROR=2
    DBG_NOTICE=3
    
    def setUi(self,ui,app):
        self.ui=ui
        self.weborf=whelper.weborf_runner(self)
        self.started=False
        self.app=app
        self.redirection=None
        
        if self.weborf.has_capability('version')>= '0.13':
            self.ui.chkTar.setEnabled(True)
        else:
            self.ui.chkTar.setEnabled(False)
        
        '''chkWrite.enabled depends on the status of chkDav. Disabling this
        it will never become enabled either'''
        self.ui.chkDav.setEnabled(self.weborf.has_capability('webdav'))
        
        #Listing addresses
        for i in nhelper.getaddrs(self.weborf.has_capability('socket')=='IPv6'):
            self.ui.cmbAddress.addItem(i,None)
        
        self.defaultdir=str(QtGui.QDesktopServices.storageLocation(QtGui.QDesktopServices.HomeLocation))
        self.ui.txtPath.setText(self.defaultdir)
        
        #TODO it could be installed in another path
        self.ui.chkNAT.setVisible(os.path.exists('/usr/bin/external-ip'))
            
    def logger(self,data,level=DBG_DEFAULT):
        '''logs an entry, showing it in the GUI'''
        if level==self.DBG_WARNING:
            data='<font color="orange"><strong>WARNING</strong></font>: %s' % data
        elif level==self.DBG_ERROR:
            data='<font color="red"><strong>ERROR</strong></font>: %s' % data
        elif level==self.DBG_NOTICE:
            data='<font color="green"><strong>NOTICE</strong></font>: %s' % data
        self.ui.txtLog.moveCursor(QtGui.QTextCursor.End)
        self.ui.txtLog.insertHtml(data+'<br>')
        self.ui.txtLog.moveCursor(QtGui.QTextCursor.End)
    
    def setDefaultValues(self):
        '''Sets default values into the form GUI. It has to be
        called after the form has been initialized'''
        pass
                        
    def stop_sharing(self):
        if self.redirection!=None:
            self.logger('Removing NAT redirection...')
            self.app.processEvents()
            self.redirection.remove_redirection()
        if self.weborf.stop():
            self.ui.cmdStart.setEnabled(True)
            self.ui.cmdStop.setEnabled(False)
            self.ui.tabWidget.setEnabled(True)
            self.started=False
        
    def about(self):
        
        self.logger('<hr><strong>Qweborf 0.14</strong>')
        self.logger('This program comes with ABSOLUTELY NO WARRANTY.'
                    ' This is free software, and you are welcome to redistribute it under certain conditions.'
                    ' For details see the <a href="http://www.gnu.org/licenses/gpl.html">GPLv3 Licese</a>.')
        self.logger('<a href="http://galileo.dmi.unict.it/wiki/weborf">Homepage</a>')
        self.logger('Salvo \'LtWorf\' Tomaselli <a href="mailto:tiposchi@tiscali.it">&lt;tiposchi@tiscali.it&gt;</a>')
        
    def terminate(self):
        if self.started:
            self.stop_sharing()
        sys.exit(0)
        
    def start_sharing(self):
        
        options={} #Dictionary with the chosen options
        
        options['path']=self.ui.txtPath.text()

        if self.ui.chkEnableAuth.isChecked():
            options['username']=self.ui.txtUsername.text()
            options['password']=self.ui.txtPassword.text()
        else:
            options['username']=None
            options['password']=None
            
        options['port'] = self.ui.spinPort.value()
        
        options['dav'] = self.ui.chkDav.isChecked()
        if self.ui.chkDav.isChecked():
            options['write'] = self.ui.chkWrite.isChecked()
        else:
            options['write'] = False
        
        options['tar'] = self.ui.chkTar.isChecked()
        
        if self.ui.cmbAddress.currentIndex()==0:
            options['ip']=None
        else:
            options['ip']=str(self.ui.cmbAddress.currentText())
            
        if self.weborf.start(options):
            self.ui.cmdStart.setEnabled(False)
            self.ui.cmdStop.setEnabled(True)
            self.ui.tabWidget.setEnabled(False)
            self.started=True
            self.redirection=None
        
        if self.ui.chkNAT.isChecked() and self.started==True:
            self.app.processEvents()
            self.logger('Trying to use UPnP to open a redirection in the NAT device. Please wait...')
            self.app.processEvents()
            external_addr=nhelper.externaladdr()
            self.app.processEvents()
            self.logger('Public IP address %s' % str(external_addr))
            self.app.processEvents()
            if external_addr!=None:
                redirection=nhelper.open_nat(options['port'])
                if redirection!=None:
                    self.redirection=redirection
                    url='http://%s:%d/' % (external_addr,redirection.eport)
                    logentry='Public address: <a href="%s">%s</a>' % (url,url)
                    self.logger(logentry)
        
    def select_path(self):
        #filename = QtGui.QFileDialog
        #dial=QtGui.QFileDialog()
        dirname= QtGui.QFileDialog.getExistingDirectory(
            self,
            QtGui.QApplication.translate("Form", "Directory to share", None, QtGui.QApplication.UnicodeUTF8),
            self.defaultdir,
            QtGui.QFileDialog.ShowDirsOnly
        )
        
        self.ui.txtPath.setText(dirname)

def q_main():
    import sys
    app = QtGui.QApplication(sys.argv)
    Form = qweborfForm()
    ui = main.Ui_Form()
    ui.setupUi(Form)
    Form.setUi(ui,app)
    Form.show()
    res=app.exec_()
    Form.terminate()
    sys.exit(res)

if __name__ == "__main__":
    q_main()
