# -*- coding: utf-8 -*-
# Weborf
# Copyright (C) 2010  Salvo "LtWorf" Tomaselli
# 
# Relational is free software: you can redistribute it and/or modify
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

class qweborfForm (QtGui.QWidget):
    '''This class is the form used for the survey, needed to intercept the events.
    It also sends the data with http POST to a page hosted on galileo'''
    def setUi(self,ui):
        self.ui=ui
    def setDefaultValues(self):
        '''Sets default values into the form GUI. It has to be
        called after the form has been initialized'''
        pass
    def auth_toggle(self,state):
        self.ui.txtPassword.setEnabled(state)
        self.ui.txtUsername.setEnabled(state)
    def stop_sharing(self):
        print "stop"
    def start_sharing(self):
        print "start"
    def select_path(self):
        print "select"

if __name__ == "__main__":
    import sys
    app = QtGui.QApplication(sys.argv)
    Form = qweborfForm()
    ui = main.Ui_Form()
    ui.setupUi(Form)
    Form.setUi(ui)
    Form.show()
    sys.exit(app.exec_())
