object Form1: TForm1
  Left = 0
  Top = 0
  Align = alClient
  BorderIcons = [biSystemMenu, biMaximize]
  Caption = 'Neon Arena'
  ClientHeight = 1041
  ClientWidth = 923
  Color = clBlack
  Constraints.MaxHeight = 1200
  Constraints.MaxWidth = 2000
  Constraints.MinHeight = 1080
  Constraints.MinWidth = 100
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'Segoe UI'
  Font.Style = []
  KeyPreview = True
  OnCreate = FormCreate
  OnKeyDown = FormKeyDown
  OnKeyUp = FormKeyUp
  TextHeight = 15
  object GameCanvas: TPaintBox
    Left = 0
    Top = 0
    Width = 923
    Height = 1041
    Align = alClient
    OnMouseDown = GameCanvasMouseDown
    OnMouseMove = GameCanvasMouseMove
    OnMouseUp = GameCanvasMouseUp
    OnPaint = GameCanvasPaint
    ExplicitWidth = 105
    ExplicitHeight = 105
  end
  object GameTimer: TTimer
    Enabled = False
    Interval = 16
    OnTimer = GameTimerTimer
  end
end
