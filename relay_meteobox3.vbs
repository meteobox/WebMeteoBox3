DeviceIP="192.168.1.222" ' IP WebMeteoBox
Password="admin"
RelayNum=0 ' Relay Number

Set oHTTP = CreateObject("winhttp.winhttprequest.5.1") 
sSource = "http://" &DeviceIP& "/lg?pa=" &Password
oHTTP.open "GET", sSource, False 
oHTTP.send
oHTTP.WaitForResponse
'WScript.Echo oHTTP.responseText

Set objRegExp = CreateObject("VBScript.RegExp")
Str = oHTTP.responseText & vbCrLf 
objRegExp.Pattern = "(\d)(\d)(\d)\w+"
Set objMatches = objRegExp.Execute(Str)
Set objMatch = objMatches.Item(0)
'MsgBox Trim(objMatch.Value)


sSource = "http://" &DeviceIP& "/rel?snd=On%2FOff&num=" & RelayNum & "&sid=" &objMatch.Value
oHTTP.open "GET", sSource, False 
oHTTP.send


