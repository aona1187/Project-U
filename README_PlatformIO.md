# M5Atom + KXR94-2050 バーコントローラー

## プロジェクト構成
```
Project-U/
├── platformio.ini     # M5Atom用プロジェクト設定
├── src/
│   └── main.cpp      # M5Atom + KXR94-2050 デュアルセンサーコード
└── README_PlatformIO.md  # このファイル
```

## ハードウェア構成
- **M5Atom x2**: WiFi通信可能なマイコン
- **KXR94-2050 x2**: 3軸加速度センサーモジュール
- **構成**: バーの両端に (M5Atom + KXR94-2050) をセット

## 1. PlatformIOでビルド・アップロード

### VS Codeでの操作手順：
1. **Ctrl+Shift+P** でコマンドパレットを開く
2. `PlatformIO: Build` と入力してビルド実行
3. `PlatformIO: Upload` でArduinoにアップロード
4. `PlatformIO: Serial Monitor` でシリアル出力を確認

### 或いは：
- VS Code下部のステータスバーで PlatformIO アイコンをクリック
- 左サイドバーの「PlatformIO」タブから操作

## 2. ハードウェア接続

### KXR94-2050 → M5Atom の配線：
```
センサー1 (バー左端):
  VCC → 3.3V
  GND → GND
  X軸 → GPIO32
  Y軸 → GPIO33
  Z軸 → GPIO25

センサー2 (バー右端):
  VCC → 3.3V
  GND → GND
  X軸 → GPIO26
  Y軸 → GPIO19
  Z軸 → GPIO27
```

### システム構成：
```
バー ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    │                              │
[KXR94-2050#1]              [KXR94-2050#2]
    └─────────── M5Atom ──────────────┘
                    │
              シリアル通信
                    │
                Unity PC
```

### デバイス設定：
- **M5Atom x1**: 単一デバイスで2つのセンサーを制御
- **KXR94-2050 x2**: アナログ出力の3軸加速度センサー
- **通信**: シリアル通信（115200 bps）でCSVデータ送信

## 3. シリアル出力フォーマット

起動時：
```
M5Atom + KXR94-2050 Bar Controller
Calibrating sensors (keep still)...
Calibration complete!
time_ms,ax1,ay1,az1,ax2,ay2,az2
```

データ出力（CSV形式、100Hz）：
```
12345,0.0123,-0.0045,0.9876,0.0234,-0.0156,1.0012
12355,0.0134,-0.0043,0.9877,0.0245,-0.0154,1.0015
...
```

### データ仕様：
- **サンプリング周波数**: 100Hz (10ms間隔)
- **加速度範囲**: 約±3g (KXR94-2050の仕様)
- **フィルタ**: ローパスフィルタ適用済み（α=0.6）
- **キャリブレーション**: 起動時に自動実行（重力補正含む）

## 4. Unity連携用のC#スクリプト例

```csharp
using System;
using System.IO.Ports;
using UnityEngine;

public class M5AtomAccelerometer : MonoBehaviour
{
    [SerializeField] private string portName = "COM3"; // M5Atomのポート
    [SerializeField] private int baudRate = 115200;
    
    private SerialPort serialPort;
    
    // 最新の加速度データ（バー両端）
    public Vector3 accel1 { get; private set; }  // 左端センサー
    public Vector3 accel2 { get; private set; }  // 右端センサー
    
    void Start()
    {
        try 
        {
            serialPort = new SerialPort(portName, baudRate);
            serialPort.Open();
            Debug.Log($"M5Atom connected on {portName}");
        }
        catch (Exception e)
        {
            Debug.LogError($"M5Atom connection failed: {e.Message}");
        }
    }
    
    void Update()
    {
        if (serialPort != null && serialPort.IsOpen && serialPort.BytesToRead > 0)
        {
            try
            {
                string data = serialPort.ReadLine();
                ParseCSV(data);
            }
            catch (Exception e)
            {
                Debug.LogWarning($"Parse error: {e.Message}");
            }
        }
    }
    
    void ParseCSV(string csvLine)
    {
        string[] values = csvLine.Split(',');
        if (values.Length >= 7)
        {
            // time_ms,ax1,ay1,az1,ax2,ay2,az2
            if (float.TryParse(values[1], out float ax1) &&
                float.TryParse(values[2], out float ay1) &&
                float.TryParse(values[3], out float az1) &&
                float.TryParse(values[4], out float ax2) &&
                float.TryParse(values[5], out float ay2) &&
                float.TryParse(values[6], out float az2))
            {
                accel1 = new Vector3(ax1, ay1, az1);
                accel2 = new Vector3(ax2, ay2, az2);
            }
        }
    }
    
    void OnDestroy()
    {
        if (serialPort != null && serialPort.IsOpen)
        {
            serialPort.Close();
        }
    }
}
```

## 5. ブロック崩しでの使用例

```csharp
public class AccelPaddle : MonoBehaviour
{
    [SerializeField] private M5AtomAccelerometer m5atom;
    [SerializeField] private float sensitivity = 5.0f;
    
    void Update()
    {
        // バーの両端の加速度差を利用してパドルを回転・移動
        Vector3 diff = m5atom.accel2 - m5atom.accel1;
        
        // X軸差分でパドル傾き制御
        float tilt = diff.x * sensitivity;
        transform.rotation = Quaternion.Euler(0, 0, tilt);
        
        // Y軸差分でパドル位置制御
        float move = diff.y * sensitivity;
        transform.Translate(move * Time.deltaTime, 0, 0);
        
        // 平均加速度で全体移動も制御可能
        Vector3 avgAccel = (m5atom.accel1 + m5atom.accel2) * 0.5f;
        // transform.Translate(avgAccel.x * Time.deltaTime, 0, 0);
    }
}
```

## トラブルシューティング

- **センサが読めない**: アナログピンの配線を確認
- **シリアルが読めない**: COMポート番号とボーレート(115200)を確認
- **データが不安定**: キャリブレーション中は絶対に動かさない
- **ビルドエラー**: `platformio.ini` でM5Atomライブラリを確認
- **ADCエラー**: ESP32のADC設定（12bit, 3.3V範囲）を確認
- **値がおかしい**: KXR94-2050の電源電圧（3.3V）を確認