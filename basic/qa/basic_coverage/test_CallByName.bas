' This file is part of the LibreOffice project.
'
' This Source Code Form is subject to the terms of the Mozilla Public
' License, v. 2.0. If a copy of the MPL was not distributed with this
' file, You can obtain one at http://mozilla.org/MPL/2.0/.
'

Option Explicit

Sub DEV : MsgBox doUnitTest : End Sub

Function doUnitTest() As String
    TestUtil.TestInit
    verify_CallByName
    doUnitTest = TestUtil.GetResult()
End Function

Sub verify_CallByName()
    Const _Get = 2, _Let = 4, _Method = 1, _Set = 8
    Dim c As New Collection
    Dim THIS_MODULE As Object : THIS_MODULE = TestModule ' cf. basic/qa/cppunit/basic_coverage.cxx

    On Error GoTo errorHandler

    ' Basic modules are Objects
    TestUtil.AssertEqual(CallByName(TestUtil, "failCount", _Get), 0, "CallByName(TestUtil, ""failCount"", _Get")
    TestUtil.AssertEqual(CallByName(TestUtil, "passCount", _Get), 1, "CallByName(TestUtil, ""passCount"", _Get")
    TestUtil.AssertEqual(CallByName(TestUtil, "GetResult", _Method), "OK", "CallByName(TestUtil, ""GetResult"", _Method")

    c.Add(100, "1st")
    c.Add(Array(7,8,9))
    TestUtil.AssertEqual(CallByName(c,"Count", _Get),2, "CallByName(c,""Count"", _Get)")
    c.Remove(2) ' remove 2nd item, 1 item left
    ' Let's check CallByName argument names
    TestUtil.AssertEqual(CallByName(callType:=_Get, object:=c, procName:="Count"), 1, _
        "CallByName(callType:=_Get, object:=c, procName:=""Count"")")

_tdf138155_:
    TestUtil.AssertEqual(CallByName(THIS_MODULE,"setArray", _Method)(3), "fox…", _
        "CallByName(THIS_MODULE,""setArray"", _Method)")

    ' ClassModule instances or UNO services are 'CallByNamable' objects too!
    ' see https://help.libreoffice.org/7.4/en-US/text/sbasic/shared/CallByName.html?DbPAR=BASIC#bm_id3150669
    ' for more _Let and _Set examples

    Exit Sub

errorHandler:
    TestUtil.ReportErrorHandler("verify_CallByName", Err, Error$, Erl)
    Resume Next

End Sub

Function setArray() ' as String[]
    setArray = Split("The quick brown fox…", " ")
End Function
