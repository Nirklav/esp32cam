# esp32cam

Firmware for esp32cam that allows it to work as a Wi-Fi camera with recording on a SD card.

## Format:
There are two kind of server methods: simple request response and stream.
All numbers in little endian encoding.

### Request:
```
[method id: 4byte int]
[size: 4byte int] // only content, without header
[content type: 4byte int] // 1 - json, 2 - binary
[content: [size]]
```

### Simple response:
```
[size: 4byte int]
[content type: 4byte int]
[content: [size]]
```

### Stream response:
```
[size: 4byte int]
[content: [size]]

[size: 4byte int]
[content: [size]]

[size: 4byte int]
[content: [size]]

...
```

## Server API:
* Get records:
  method id: 0
  content-type: json
  content example: {"password":"123456"} 
* Get record:
  method id: 1
  content-type: json
  content example: {"file":"<file-name-from-get-records>", "password":"123456"}
  response: avi file in mjpeg format
* Stream camera:
  method id: 2
  content-type: json
  content example: {"password":"123456"}
  response: jpeg images
* Restart:
  method id: 3
  content-type: json
  content example: {"password":"123456"}

Client example C# WinForms:
``` C#
public ViewerForm()
{
  InitializeComponent();

  var thread = new Thread(Update);
  thread.IsBackground = true;
  thread.Start();
}

private void Update()
{
  using (var client = new TcpClient())
  {
    client.NoDelay = true;
    client.Connect("<ip>", 1021);
    var stream = client.GetStream();

    var input = Encoding.UTF8.GetBytes("{\"password\":\"123456\"}");

    // method id:
    Write(stream, BitConverter.GetBytes(2));
    
    // input size:
    Write(stream, BitConverter.GetBytes(input.Length));
    
    // content-type:
    Write(stream, BitConverter.GetBytes(1));
    
    // content:
    Write(stream, input);

    while (true)
    {
      var sizeBuffer = new byte[4];
    
      // Read output size
      stream.Read(sizeBuffer, 0, 4);
      var size = BitConverter.ToInt32(sizeBuffer, 0);
      var read = 0;
      
      // Read image jpeg
      var imageBlob = new byte[size];
      while (read < size)
        read += stream.Read(imageBlob, read, size - read);
    
      var image = Image.FromStream(new MemoryStream(imageBlob));

      // picture is System.Windows.Forms.PictureBox
      picture.Invoke((Action)(() => picture.Image = image));
    }
  }
}

private static void Write(Stream stream, byte[] data) =>
  stream.Write(data, 0, data.Length);
```
