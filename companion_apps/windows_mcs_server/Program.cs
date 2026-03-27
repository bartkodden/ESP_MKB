using System;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows.Forms;
using Windows.Devices.Bluetooth;
using Windows.Devices.Bluetooth.GenericAttributeProfile;
using Windows.Media.Control;
using Windows.Storage.Streams;
using Windows.Security.Cryptography;
using Windows.Graphics.Imaging;
using NAudio.CoreAudioApi;

namespace WindowsMCSServer
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new TrayApplicationContext());
        }
    }

    public class TrayApplicationContext : ApplicationContext
    {
        private NotifyIcon trayIcon;
        private MCSServerForm? serverForm;
        private MCSServer? server;
        private int restartCount = 0;
        private Icon appIcon;

        public TrayApplicationContext()
        {
            appIcon = LoadIcon();
            
            trayIcon = new NotifyIcon()
            {
                Icon = appIcon,
                ContextMenuStrip = new ContextMenuStrip(),
                Visible = true,
                Text = "MCS Server - Starting..."
            };

            trayIcon.ContextMenuStrip.Items.Add("Show", null, (s, e) => ShowForm());
            trayIcon.ContextMenuStrip.Items.Add("Quit", null, (s, e) => ExitApp());
            
            trayIcon.DoubleClick += (s, e) => ShowForm();

            serverForm = new MCSServerForm(this, appIcon);
            
            StartServer();
        }

        private Icon LoadIcon()
        {
            try
            {
                var assembly = System.Reflection.Assembly.GetExecutingAssembly();
                using (var stream = assembly.GetManifestResourceStream("WindowsMCSServer.mcs_icon.ico"))
                {
                    if (stream != null) return new Icon(stream);
                }
            }
            catch { }
            
            return SystemIcons.Application;
        }

        private async void StartServer()
        {
            restartCount++;
            
            if (restartCount > 1)
            {
                UpdateUI($"Restarting (#{restartCount})...", "Please wait", false, 0, 0);
                await Task.Delay(2000);
            }
            
            try
            {
                server = new MCSServer(UpdateUI);
                await server.StartAsync();
            }
            catch (Exception ex)
            {
                UpdateUI($"Error: {ex.Message}", "Server failed", false, 0, 0);
                File.AppendAllText("crash.log", $"[{DateTime.Now}] {ex.Message}\n");
                
                await Task.Delay(3000);
                StartServer();
            }
        }

        public void RestartServer()
        {
            UpdateUI("Restarting...", "Please wait", false, 0, 0);
            StartServer();
        }

        public void UpdateUI(string status, string track, bool connected, int volume, int mic)
        {
            string shortTrack = track.Length > 40 ? track.Substring(0, 37) + "..." : track;
            trayIcon.Text = connected ? $"MCS - {shortTrack}" : "MCS - Disconnected";
            serverForm?.UpdateStatus(status, track, connected, volume, mic);
        }

        private void ShowForm()
        {
            if (serverForm == null || serverForm.IsDisposed)
            {
                serverForm = new MCSServerForm(this, appIcon);
            }
            serverForm.Show();
            serverForm.WindowState = FormWindowState.Normal;
            serverForm.Activate();
        }

        private void ExitApp()
        {
            trayIcon.Visible = false;
            Application.Exit();
        }
    }

    public class MCSServerForm : Form
    {
        private TrayApplicationContext context;
        private Label connectionLabel;
        private Label trackLabel;
        private Label statusLabel;
        private ProgressBar volumeBar;
        private ProgressBar micBar;
        private Label volumeLabel;
        private Label micLabel;
        private Button restartButton;
        private Button quitButton;

        public MCSServerForm(TrayApplicationContext ctx, Icon icon)
        {
            context = ctx;
            
            Text = "MCS BLE Server v2.1";
            Size = new Size(450, 400);
            StartPosition = FormStartPosition.CenterScreen;
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = false;
            BackColor = Color.FromArgb(245, 245, 245);
            Icon = icon;
            
            // Title
            var titleLabel = new Label
            {
                Text = "Windows MCS BLE Server",
                Location = new Point(20, 15),
                Size = new Size(400, 25),
                Font = new Font("Segoe UI", 12, FontStyle.Bold)
            };
            Controls.Add(titleLabel);

            // Connection status
            connectionLabel = new Label
            {
                Text = "● Disconnected",
                Location = new Point(20, 50),
                Size = new Size(400, 25),
                Font = new Font("Segoe UI", 10, FontStyle.Bold),
                ForeColor = Color.Gray
            };
            Controls.Add(connectionLabel);

            // Track info
            trackLabel = new Label
            {
                Text = "No Media",
                Location = new Point(20, 85),
                Size = new Size(400, 45),
                Font = new Font("Segoe UI", 9),
                AutoSize = false
            };
            Controls.Add(trackLabel);

            // Status
            statusLabel = new Label
            {
                Text = "Initializing...",
                Location = new Point(20, 140),
                Size = new Size(400, 20),
                Font = new Font("Segoe UI", 8),
                ForeColor = Color.Gray
            };
            Controls.Add(statusLabel);

            // Volume label
            volumeLabel = new Label
            {
                Text = "🔊 Volume: 0%",
                Location = new Point(20, 170),
                Size = new Size(150, 20),
                Font = new Font("Segoe UI", 9)
            };
            Controls.Add(volumeLabel);

            // Volume bar
            volumeBar = new ProgressBar
            {
                Location = new Point(20, 195),
                Size = new Size(400, 20),
                Maximum = 100,
                Value = 0
            };
            Controls.Add(volumeBar);

            // Mic label
            micLabel = new Label
            {
                Text = "🎤 Microphone: 0%",
                Location = new Point(20, 225),
                Size = new Size(150, 20),
                Font = new Font("Segoe UI", 9)
            };
            Controls.Add(micLabel);

            // Mic bar
            micBar = new ProgressBar
            {
                Location = new Point(20, 250),
                Size = new Size(400, 20),
                Maximum = 100,
                Value = 0
            };
            Controls.Add(micBar);

            // Restart button
            restartButton = new Button
            {
                Text = "Restart Server",
                Location = new Point(20, 285),
                Size = new Size(120, 35),
                Font = new Font("Segoe UI", 9)
            };
            restartButton.Click += (s, e) => context.RestartServer();
            Controls.Add(restartButton);

            // Quit button
            quitButton = new Button
            {
                Text = "Quit",
                Location = new Point(310, 285),
                Size = new Size(120, 35),
                Font = new Font("Segoe UI", 9)
            };
            quitButton.Click += (s, e) => Application.Exit();
            Controls.Add(quitButton);

            FormClosing += (s, e) =>
            {
                if (e.CloseReason == CloseReason.UserClosing)
                {
                    e.Cancel = true;
                    Hide();
                }
            };
        }

        public void UpdateStatus(string status, string track, bool connected, int volume, int mic)
        {
            if (InvokeRequired)
            {
                Invoke(new Action(() => UpdateStatus(status, track, connected, volume, mic)));
                return;
            }

            connectionLabel.Text = connected ? "● Connected" : "● Disconnected";
            connectionLabel.ForeColor = connected ? Color.Green : Color.Red;
            trackLabel.Text = track;
            statusLabel.Text = status;
            
            // Grey out bars when disconnected
            volumeBar.Value = Math.Clamp(volume, 0, 100);
            volumeLabel.Text = $"🔊 Volume: {volume}%";
            volumeBar.Enabled = connected;
            volumeLabel.ForeColor = connected ? Color.Black : Color.Gray;
            
            micBar.Value = Math.Clamp(mic, 0, 100);
            micLabel.Text = $"🎤 Microphone: {mic}%";
            micBar.Enabled = connected;
            micLabel.ForeColor = connected ? Color.Black : Color.Gray;
        }
    }

    // Windows Audio Control (P/Invoke)
    public class WindowsAudioControl
    {
        private static MMDeviceEnumerator deviceEnumerator = new MMDeviceEnumerator();
        
        public static void SetVolume(int percent)
        {
            try
            {
                var device = deviceEnumerator.GetDefaultAudioEndpoint(DataFlow.Render, Role.Multimedia);
                float volume = Math.Clamp(percent / 100f, 0f, 1f);
                device.AudioEndpointVolume.MasterVolumeLevelScalar = volume;
            }
            catch { }
        }

        public static void SetMicrophoneVolume(int percent)
        {
            try
            {
                var device = deviceEnumerator.GetDefaultAudioEndpoint(DataFlow.Capture, Role.Communications);
                float volume = Math.Clamp(percent / 100f, 0f, 1f);
                device.AudioEndpointVolume.MasterVolumeLevelScalar = volume;
            }
            catch { }
        }
        
        public static int GetVolume()
        {
            try
            {
                var device = deviceEnumerator.GetDefaultAudioEndpoint(DataFlow.Render, Role.Multimedia);
                return (int)(device.AudioEndpointVolume.MasterVolumeLevelScalar * 100);
            }
            catch { return 50; }
        }
        
        public static int GetMicrophoneVolume()
        {
            try
            {
                var device = deviceEnumerator.GetDefaultAudioEndpoint(DataFlow.Capture, Role.Communications);
                return (int)(device.AudioEndpointVolume.MasterVolumeLevelScalar * 100);
            }
            catch { return 50; }
        }
    }

    public class MCSServer
    {
        // MCS UUIDs
        private static readonly Guid MCS_SERVICE_UUID = new Guid("00001848-0000-1000-8000-00805f9b34fb");
        private static readonly Guid MEDIA_STATE_UUID = new Guid("00002ba3-0000-1000-8000-00805f9b34fb");
        private static readonly Guid TRACK_TITLE_UUID = new Guid("00002b97-0000-1000-8000-00805f9b34fb");
        private static readonly Guid TRACK_POS_UUID = new Guid("00002b99-0000-1000-8000-00805f9b34fb");
        private static readonly Guid TRACK_DURATION_UUID = new Guid("00002b98-0000-1000-8000-00805f9b34fb");
        private static readonly Guid MCS_VOLUME_UUID = new Guid("00002b7e-0000-1000-8000-00805f9b34fb");
        
        // VCS UUIDs
        private static readonly Guid VCS_SERVICE_UUID = new Guid("00001844-0000-1000-8000-00805f9b34fb");
        private static readonly Guid VCS_VOLUME_STATE_UUID = new Guid("00002b7d-0000-1000-8000-00805f9b34fb");
        
        // Custom Microphone UUID
        private static readonly Guid MICROPHONE_UUID = new Guid("00002c7f-0000-1000-8000-00805f9b34fb");
        
        private static readonly Guid[] CHUNK_UUIDS = new Guid[16]
        {
            new Guid("00002bf0-0000-1000-8000-00805f9b34fb"), new Guid("00002bf1-0000-1000-8000-00805f9b34fb"),
            new Guid("00002bf2-0000-1000-8000-00805f9b34fb"), new Guid("00002bf3-0000-1000-8000-00805f9b34fb"),
            new Guid("00002bf4-0000-1000-8000-00805f9b34fb"), new Guid("00002bf5-0000-1000-8000-00805f9b34fb"),
            new Guid("00002bf6-0000-1000-8000-00805f9b34fb"), new Guid("00002bf7-0000-1000-8000-00805f9b34fb"),
            new Guid("00002bf8-0000-1000-8000-00805f9b34fb"), new Guid("00002bf9-0000-1000-8000-00805f9b34fb"),
            new Guid("00002bfa-0000-1000-8000-00805f9b34fb"), new Guid("00002bfb-0000-1000-8000-00805f9b34fb"),
            new Guid("00002bfc-0000-1000-8000-00805f9b34fb"), new Guid("00002bfd-0000-1000-8000-00805f9b34fb"),
            new Guid("00002bfe-0000-1000-8000-00805f9b34fb"), new Guid("00002bff-0000-1000-8000-00805f9b34fb"),
        };

        private GattServiceProvider? mcsServiceProvider;
        private GattServiceProvider? vcsServiceProvider;
        
        // MCS Characteristics
        private GattLocalCharacteristic? mediaStateChar, trackTitleChar, trackPosChar, trackDurationChar, mcsVolumeChar;
        private GattLocalCharacteristic[] albumChunkChars = new GattLocalCharacteristic[16];
        
        // VCS Characteristics
        private GattLocalCharacteristic? vcsVolumeChar;
        
        // Custom Microphone
        private GattLocalCharacteristic? microphoneChar;

        private byte currentState = 0x00;
        private string currentTitle = "No Media", lastTitle = "", lastAlbumTrack = "";
        private uint currentPosition = 0;
        private uint currentDuration = 0, lastDuration = 0;
        private byte lastState = 0xFF;
        private byte currentVolume = 50;
        private byte currentMicrophone = 50;
        
        private byte[][] chunks = new byte[16][];
        private Action<string, string, bool, int, int> uiCallback;

        public MCSServer(Action<string, string, bool, int, int> callback)
        {
            uiCallback = callback;
        }

        public async Task StartAsync()
        {
            for (int i = 0; i < 16; i++) chunks[i] = new byte[512];

            var adapter = await BluetoothAdapter.GetDefaultAsync();
            if (adapter?.IsPeripheralRoleSupported != true) 
            {
                uiCallback?.Invoke("Error: Bluetooth not supported", "No adapter", false, 0, 0);
                return;
            }

            // Create MCS Service
            var mcsResult = await GattServiceProvider.CreateAsync(MCS_SERVICE_UUID);
            if (mcsResult.Error != BluetoothError.Success) 
            {
                uiCallback?.Invoke("Error: Failed to create MCS", "BLE error", false, 0, 0);
                return;
            }
            mcsServiceProvider = mcsResult.ServiceProvider;
            
            // Create VCS Service
            var vcsResult = await GattServiceProvider.CreateAsync(VCS_SERVICE_UUID);
            if (vcsResult.Error != BluetoothError.Success) 
            {
                uiCallback?.Invoke("Error: Failed to create VCS", "BLE error", false, 0, 0);
                return;
            }
            vcsServiceProvider = vcsResult.ServiceProvider;
            
            if (!await CreateMCSCharacteristicsAsync()) return;
            if (!await CreateVCSCharacteristicsAsync()) return;
            
            await Task.Delay(500);

            // Start advertising both services
            mcsServiceProvider.StartAdvertising(new GattServiceProviderAdvertisingParameters 
            { 
                IsDiscoverable = true, 
                IsConnectable = true 
            });
            
            vcsServiceProvider.StartAdvertising(new GattServiceProviderAdvertisingParameters 
            { 
                IsDiscoverable = true, 
                IsConnectable = true 
            });
            
            await Task.Delay(1000);
            uiCallback?.Invoke("Ready - Advertising MCS & VCS", "Waiting for connection", false, 50, 50);

            _ = MonitorMediaAsync();
        }

        private async Task<bool> CreateMCSCharacteristicsAsync()
        {
            var p = new GattLocalCharacteristicParameters
            {
                CharacteristicProperties = GattCharacteristicProperties.Read | GattCharacteristicProperties.Notify,
                ReadProtectionLevel = GattProtectionLevel.Plain
            };

            var r1 = await mcsServiceProvider!.Service.CreateCharacteristicAsync(MEDIA_STATE_UUID, p);
            if (r1.Error != BluetoothError.Success) return false;
            mediaStateChar = r1.Characteristic;
            mediaStateChar.ReadRequested += async (s, a) => await SafeRead(a, currentState);

            var r2 = await mcsServiceProvider.Service.CreateCharacteristicAsync(TRACK_TITLE_UUID, p);
            if (r2.Error != BluetoothError.Success) return false;
            trackTitleChar = r2.Characteristic;
            trackTitleChar.ReadRequested += async (s, a) => await SafeRead(a, currentTitle);

            var r3 = await mcsServiceProvider.Service.CreateCharacteristicAsync(TRACK_POS_UUID, p);
            if (r3.Error != BluetoothError.Success) return false;
            trackPosChar = r3.Characteristic;
            trackPosChar.ReadRequested += async (s, a) => await SafeRead(a, currentPosition);

            var r4 = await mcsServiceProvider.Service.CreateCharacteristicAsync(TRACK_DURATION_UUID, p);
            if (r4.Error != BluetoothError.Success) return false;
            trackDurationChar = r4.Characteristic;
            trackDurationChar.ReadRequested += async (s, a) => await SafeRead(a, currentDuration);

            // MCS Volume (Read + Write)
            var volumeParams = new GattLocalCharacteristicParameters
            {
                CharacteristicProperties = GattCharacteristicProperties.Read | 
                                         GattCharacteristicProperties.Write | 
                                         GattCharacteristicProperties.Notify,
                ReadProtectionLevel = GattProtectionLevel.Plain,
                WriteProtectionLevel = GattProtectionLevel.Plain
            };
            
            var r5 = await mcsServiceProvider.Service.CreateCharacteristicAsync(MCS_VOLUME_UUID, volumeParams);
            if (r5.Error != BluetoothError.Success) return false;
            mcsVolumeChar = r5.Characteristic;
            mcsVolumeChar.ReadRequested += async (s, a) => await SafeRead(a, currentVolume);
            mcsVolumeChar.WriteRequested += OnVolumeWriteAsync;

            // DEBUG: Verify callback registered
            File.AppendAllText("startup.log", $"MCS Volume char created: {mcsVolumeChar != null}\n");
            File.AppendAllText("startup.log", $"  Properties: {volumeParams.CharacteristicProperties}\n");

            // Album chunks
            var cp = new GattLocalCharacteristicParameters
            {
                CharacteristicProperties = GattCharacteristicProperties.Read,
                ReadProtectionLevel = GattProtectionLevel.Plain
            };

            for (int i = 0; i < 16; i++)
            {
                var r = await mcsServiceProvider.Service.CreateCharacteristicAsync(CHUNK_UUIDS[i], cp);
                if (r.Error == BluetoothError.Success)
                {
                    int idx = i;
                    albumChunkChars[i] = r.Characteristic;
                    albumChunkChars[i].ReadRequested += async (s, a) => await ReadChunk(a, idx);
                }
            }
            
            return true;
        }

        private async Task<bool> CreateVCSCharacteristicsAsync()
        {
            // VCS Volume State (Read + Write + Notify)
            var volumeParams = new GattLocalCharacteristicParameters
            {
                CharacteristicProperties = GattCharacteristicProperties.Read | 
                                         GattCharacteristicProperties.Write | 
                                         GattCharacteristicProperties.Notify,
                ReadProtectionLevel = GattProtectionLevel.Plain,
                WriteProtectionLevel = GattProtectionLevel.Plain
            };
            
            var r1 = await vcsServiceProvider!.Service.CreateCharacteristicAsync(VCS_VOLUME_STATE_UUID, volumeParams);
            if (r1.Error != BluetoothError.Success) return false;
            vcsVolumeChar = r1.Characteristic;
            vcsVolumeChar.ReadRequested += async (s, a) => await SafeRead(a, currentVolume);
            vcsVolumeChar.WriteRequested += OnVolumeWriteAsync;
            
            // Custom Microphone (Read + Write)
            var r2 = await vcsServiceProvider.Service.CreateCharacteristicAsync(MICROPHONE_UUID, volumeParams);
            if (r2.Error != BluetoothError.Success) return false;
            microphoneChar = r2.Characteristic;
            microphoneChar.ReadRequested += async (s, a) => await SafeRead(a, currentMicrophone);
            microphoneChar.WriteRequested += OnMicrophoneWriteAsync;
            
            return true;
        }

        private async void OnVolumeWriteAsync(GattLocalCharacteristic sender, GattWriteRequestedEventArgs args)
        {
            var deferral = args.GetDeferral();
            
            File.AppendAllText("debug.log", $"{DateTime.Now}: OnVolumeWriteAsync CALLED\n");
            
            try
            {
                var request = await args.GetRequestAsync();
                File.AppendAllText("debug.log", $"  Request: {request != null}\n");
                
                if (request != null)
                {
                    var reader = DataReader.FromBuffer(request.Value);
                    File.AppendAllText("debug.log", $"  Buffer length: {reader.UnconsumedBufferLength}\n");
                    
                    if (reader.UnconsumedBufferLength >= 1)
                    {
                        byte volume = reader.ReadByte();
                        File.AppendAllText("debug.log", $"  Volume received: {volume}\n");
                        
                        currentVolume = (byte)Math.Clamp((int)volume, 0, 100);
                        File.AppendAllText("debug.log", $"  Current volume set to: {currentVolume}\n");
                        
                        _ = Task.Run(() => WindowsAudioControl.SetVolume(currentVolume));
    
                        request.Respond();
                        
                        UpdateStatusUI();
                    }
                }
            }
            catch (Exception ex)
            {
                File.AppendAllText("debug.log", $"  ERROR: {ex.Message}\n{ex.StackTrace}\n");
            }
            finally 
            { 
                File.AppendAllText("debug.log", $"  Deferral completing\n");
                deferral.Complete(); 
            }
        }

        private async void OnMicrophoneWriteAsync(GattLocalCharacteristic sender, GattWriteRequestedEventArgs args)
        {
            var deferral = args.GetDeferral();
            try
            {
                var request = await args.GetRequestAsync();
                if (request != null)
                {
                    var reader = DataReader.FromBuffer(request.Value);
                    if (reader.UnconsumedBufferLength >= 1)
                    {
                        byte mic = reader.ReadByte();
                        currentMicrophone = (byte)Math.Clamp((int)mic, 0, 100);
                        
                        _ = Task.Run(() => WindowsAudioControl.SetMicrophoneVolume(currentMicrophone));
                        
                        request.Respond();
                        
                        _ = Task.Run(() => UpdateStatusUI());
                    }
                }
            }
            catch (Exception ex)
            {
                string desktop = Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
                File.AppendAllText(Path.Combine(desktop, "volume_error.log"), $"{DateTime.Now}: {ex.Message}\n");
            }
            finally { deferral.Complete(); }
        }

        private async Task ReadChunk(GattReadRequestedEventArgs args, int idx)
        {
            var d = args.GetDeferral();
            try
            {
                var req = await args.GetRequestAsync();
                req?.RespondWithValue(CryptographicBuffer.CreateFromByteArray(chunks[idx]));
            }
            catch { }
            finally { d.Complete(); }
        }

        private async Task SafeRead(GattReadRequestedEventArgs args, object value)
        {
            var d = args.GetDeferral();
            try
            {
                var req = await args.GetRequestAsync();
                if (req != null)
                {
                    var w = new DataWriter();
                    w.ByteOrder = ByteOrder.LittleEndian;
                    
                    if (value is byte b) w.WriteByte(b);
                    else if (value is string s) w.WriteString(s);
                    else if (value is uint u) w.WriteUInt32(u);
                    req.RespondWithValue(w.DetachBuffer());
                }
            }
            catch { }
            finally { d.Complete(); }
        }

        private async Task MonitorMediaAsync()
        {
            var mgr = await GlobalSystemMediaTransportControlsSessionManager.RequestAsync();

            while (true)
            {
                try { await UpdateMediaInfoAsync(mgr); }
                catch { }
                await Task.Delay(1000);
            }
        }

        private async Task UpdateMediaInfoAsync(GlobalSystemMediaTransportControlsSessionManager? mgr)
        {
            var session = mgr?.GetCurrentSession();
            
            if (session == null)
            {
                if (currentTitle != "No Media")
                {
                    currentState = 0x00;
                    currentTitle = "No Media";
                    currentPosition = 0;
                    currentDuration = 0;
                    lastTitle = "";
                    lastAlbumTrack = "";
                    for (int i = 0; i < 16; i++) Array.Clear(chunks[i], 0, 512);
                    await SafeNotify();
                    UpdateStatusUI();
                }
                else
                {
                    // Heartbeat even with no media
                    try { if (mediaStateChar?.SubscribedClients.Count > 0) { var w = new DataWriter(); w.WriteByte(0x00); await mediaStateChar.NotifyValueAsync(w.DetachBuffer()); } } catch { }
                }
                return;
            }

            var props = await session.TryGetMediaPropertiesAsync();
            if (props == null) return;

            byte newState = session.GetPlaybackInfo().PlaybackStatus switch
            {
                GlobalSystemMediaTransportControlsSessionPlaybackStatus.Playing => (byte)0x01,
                GlobalSystemMediaTransportControlsSessionPlaybackStatus.Paused => (byte)0x02,
                _ => (byte)0x00
            };

            string newTitle = $"{props.Artist ?? "Unknown"} - {props.Title ?? "Unknown"}";
            uint newPos = 0;
            uint newDuration = 0;
            
            try 
            { 
                var timeline = session.GetTimelineProperties();
                if (timeline != null)
                {
                    newPos = (uint)Math.Max(0, timeline.Position.TotalSeconds);
                    newDuration = (uint)Math.Max(0, timeline.EndTime.TotalSeconds);
                }
            } 
            catch { }

            // Album art on track change
            if (newTitle != lastAlbumTrack && newTitle != "Unknown - Unknown")
            {
                lastAlbumTrack = newTitle;
                await Task.Delay(1000);
                
                try
                {
                    var freshProps = await session.TryGetMediaPropertiesAsync();
                    if (freshProps?.Thumbnail != null)
                    {
                        byte[] rgb = await GetResizedAlbumArt(freshProps.Thumbnail);
                        if (rgb.Length == 8192)
                        {
                            for (int i = 0; i < 16; i++) Array.Clear(chunks[i], 0, 512);
                            for (int i = 0; i < 16; i++) Array.Copy(rgb, i * 512, chunks[i], 0, 512);
                        }
                    }
                }
                catch { }
            }

            bool titleChanged = newTitle != lastTitle;
            bool stateChanged = newState != lastState;
            bool durationChanged = newDuration != lastDuration;
            
            currentTitle = newTitle;
            currentState = newState;
            currentDuration = newDuration;
            
            if (titleChanged || stateChanged || durationChanged) 
            {
                currentPosition = newPos;
                
                lastTitle = newTitle;
                lastState = newState;
                lastDuration = newDuration;
                
                await SafeNotify();
                UpdateStatusUI();
            } 
            else 
            {
                // Heartbeat
                try { if (mediaStateChar?.SubscribedClients.Count > 0) { var w = new DataWriter(); w.WriteByte(currentState); await mediaStateChar.NotifyValueAsync(w.DetachBuffer()); } } catch { }
            }
        }

        private async Task<byte[]> GetResizedAlbumArt(IRandomAccessStreamReference thumb)
        {
            try
            {
                using (var stream = await thumb.OpenReadAsync())
                {
                    var decoder = await BitmapDecoder.CreateAsync(stream);
                    var pixelData = await decoder.GetPixelDataAsync(
                        BitmapPixelFormat.Rgba8, BitmapAlphaMode.Straight,
                        new BitmapTransform { ScaledWidth = 64, ScaledHeight = 64 },
                        ExifOrientationMode.RespectExifOrientation,
                        ColorManagementMode.DoNotColorManage
                    );
                    
                    byte[] px = pixelData.DetachPixelData();
                    byte[] rgb565 = new byte[8192];
                    
                    for (int i = 0; i < 4096; i++)
                    {
                        ushort rgb = (ushort)(((px[i*4] >> 3) << 11) | ((px[i*4+1] >> 2) << 5) | (px[i*4+2] >> 3));
                        rgb565[i*2] = (byte)(rgb & 0xFF);
                        rgb565[i*2+1] = (byte)(rgb >> 8);
                    }
                    
                    return rgb565;
                }
            }
            catch { return new byte[0]; }
        }

        private void UpdateStatusUI()
        {
            var st = currentState switch { 0x01 => "Playing", 0x02 => "Paused", _ => "Stopped" };
            var conn = (mediaStateChar?.SubscribedClients.Count ?? 0) > 0;
            
            string displayTitle = currentTitle.Length > 50 ? currentTitle.Substring(0, 47) + "..." : currentTitle;
            string status = $"{st} - {currentPosition}/{currentDuration}s";
            
            uiCallback?.Invoke(status, displayTitle, conn, currentVolume, currentMicrophone);
        }

        private async Task SafeNotify()
        {
            try { if (mediaStateChar?.SubscribedClients.Count > 0) { var w = new DataWriter(); w.ByteOrder = ByteOrder.LittleEndian; w.WriteByte(currentState); await mediaStateChar.NotifyValueAsync(w.DetachBuffer()); await Task.Delay(50); } } catch { }
            try { if (trackTitleChar?.SubscribedClients.Count > 0) { var w = new DataWriter(); w.ByteOrder = ByteOrder.LittleEndian; w.WriteString(currentTitle); await trackTitleChar.NotifyValueAsync(w.DetachBuffer()); await Task.Delay(50); } } catch { }
            try { if (trackPosChar?.SubscribedClients.Count > 0) { var w = new DataWriter(); w.ByteOrder = ByteOrder.LittleEndian; w.WriteUInt32(currentPosition); await trackPosChar.NotifyValueAsync(w.DetachBuffer()); await Task.Delay(50); } } catch { }
            try { if (trackDurationChar?.SubscribedClients.Count > 0) { var w = new DataWriter(); w.ByteOrder = ByteOrder.LittleEndian; w.WriteUInt32(currentDuration); await trackDurationChar.NotifyValueAsync(w.DetachBuffer()); } } catch { }
        }
    }
}