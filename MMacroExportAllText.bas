Attribute VB_Name = "MMacroExportAllText"
Option Explicit

Public Sub ExportAllTextPPT()
    Dim outpath As String
    Dim outno As Integer
    With Application.FileDialog(msoFileDialogFolderPicker)
        .Title = "Export to directory..."
        
        If Not .Show() Then Exit Sub
        If IsEmpty(.SelectedItems) Then Exit Sub
        outpath = .SelectedItems(1)
    End With
    
    Dim ppt As Presentation
    Dim slide As slide
    Dim shape As shape
    Dim text As String
    
    For Each ppt In Presentations
        text = ""
        On Error Resume Next
        For Each slide In ppt.Slides
            For Each shape In slide.Shapes
                text = text + shape.TextFrame.TextRange.text + vbCrLf
            Next shape
        Next slide
        On Error GoTo 0
        
        outno = FreeFile
        Open outpath + "\" + Left$(ppt.Name, InStrRev(ppt.Name, ".") - 1) + ".txt" For Output As outno
        Print #outno, text
        Close outno
    Next ppt
    
    MsgBox "Done"
End Sub

Public Sub ExportAllTextDOC()
    Dim outpath As String
    Dim outno As Integer
    With Application.FileDialog(msoFileDialogFolderPicker)
        .Title = "Export to directory..."
        
        If Not .Show() Then Exit Sub
        If IsEmpty(.SelectedItems) Then Exit Sub
        outpath = .SelectedItems(1)
    End With
    
    Dim doc As Document
    Dim shape As shape
    Dim text As String
    
    For Each doc In Documents
        text = doc.Range().text
        On Error Resume Next
        For Each shape In doc.Shapes
            text = text + shape.TextFrame.TextRange.text + vbCrLf
        Next shape
        On Error GoTo 0
        
        outno = FreeFile
        Open outpath + "\" + Left$(doc.Name, InStrRev(doc.Name, ".") - 1) + ".txt" For Output As outno
        Print #outno, text
        Close outno
    Next doc
    
    MsgBox "Done"
End Sub

Public Sub TextStyleBinarization()
    Dim para As Paragraph
    Dim char As Range
    Dim stat As New Scripting.Dictionary
    Dim maxcolor As Long
    Dim i As Variant
    Dim processed As Long
    'Dim basestyle As Style
    
    processed = 0
    'Set basestyle = ThisDocument.Styles("正文")
    For Each para In ThisDocument.Paragraphs
        For Each char In para.Range.Characters
            If stat.Exists(char.Font.Color) Then
                stat(char.Font.Color) = stat(char.Font.Color) + 1
            Else
                stat.Add char.Font.Color, 1
            End If
        Next char
        
        maxcolor = stat.Keys(0)
        For Each i In stat.Keys
            If stat(i) > stat(maxcolor) Then
                maxcolor = i
            End If
        Next i
        
        For Each char In para.Range.Characters
            'char.Style = basestyle
            With char.Font
                .Color = IIf(.Color = maxcolor, wdColorBlack, wdColorBlue)
                '.Name = "黑体"
                '.Size = 12
                '.Bold = False
                '.Italic = False
                '.Underline = wdUnderlineNone
            End With
        Next char
        
        stat.RemoveAll
        processed = processed + 1
        'If processed > 10 Then Exit Sub
        If processed Mod 100 = 0 Then
            Debug.Print processed & " paragraphs processed"
            DoEvents
        End If
    Next para
    
    MsgBox processed & " paragraphs processed"
End Sub

