using HidApi;
using System.Runtime.InteropServices;

public enum MouseButton
{
    Left = 1,
    Right = 2,
    Middle = 4,
    All = Left | Right | Middle
}

public class rkMouse
{
    [DllImport("hidapi.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
    public static extern nint hid_open_path(string path);

    [DllImport("hidapi.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, SetLastError = true)]
    public static extern int hid_write(nint device, byte[] data, int length);

    [DllImport("hidapi.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
    public static extern void hid_close(nint device);

    [DllImport("hidapi.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
    public static extern int hid_read(nint device, byte[] data, int length);

    [DllImport("hidapi.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
    public static extern int hid_read_timeout(nint device, byte[] data, int length, int milliseconds);

    private int _buttonsMask = 0;
    private nint _deviceHandle;

    public rkMouse(nint deviceHandle)
    {
        _deviceHandle = deviceHandle;
        _buttonsMask = 0;
        Move(0, 0);
    }

    public static rkMouse GetMouse(int vid, int pid, byte pingCode = 0xf9)
    {
        var deviceHandle = FindMouseDeviceByVidPid(vid, pid, pingCode);
        if (deviceHandle == (nint)0)
        {
            throw new DeviceNotFoundException($"Device Vendor ID: {vid:X}, Product ID: {pid:X} not found!");
        }
        return new rkMouse(deviceHandle);
    }

    public static rkMouse GetMouse(int releaseNumber = 5666, byte pingCode = 0xf9)
    {
        var deviceHandle = FindMouseDeviceByReleaseNumber(releaseNumber, pingCode);
        if (deviceHandle == (nint)0)
        {
            throw new DeviceNotFoundException($"Device with Release Number: {releaseNumber} not found!");
        }
        return new rkMouse(deviceHandle);
    }

    private void Buttons(int buttons)
    {
        if (buttons != _buttonsMask)
        {
            _buttonsMask = buttons;
            Move(0, 0);
        }
    }

    public void Click(MouseButton button = MouseButton.Left)
    {
        _buttonsMask = (int)button;
        Move(0, 0);
        _buttonsMask = 0;
        Move(0, 0);
    }

    public void Press(MouseButton button = MouseButton.Left)
    {
        Buttons(_buttonsMask | (int)button);
    }

    public void Release(MouseButton button = MouseButton.Left)
    {
        Buttons(_buttonsMask & ~(int)button);
    }

    public bool IsPressed(MouseButton button = MouseButton.Left)
    {
        return (_buttonsMask & (int)button) != 0;
    }

    public void Move(int x, int y)
    {
        int limitedX = LimitXY(x);
        int limitedY = LimitXY(y);
        SendRawReport(MakeReport(limitedX, limitedY));
    }

    private byte[] MakeReport(int x, int y)
    {
        return new byte[]
        {
            0x01, // Report ID
            (byte)_buttonsMask,
            LowByte(x), HighByte(x),
            LowByte(y), HighByte(y)
        };
    }

    private void SendRawReport(byte[] reportData)
    {
        if (_deviceHandle == (nint)0)
        {
            throw new InvalidOperationException("Device handle is invalid.");
        }

        int bytesWritten = hid_write(_deviceHandle, reportData, reportData.Length);
        if (bytesWritten == -1)
        {
            int error = Marshal.GetLastWin32Error();
            throw new Exception($"Error writing to the device. Error code: {error}");
        }
        else if (bytesWritten < reportData.Length)
        {
            throw new Exception($"Could not write all bytes to the device. Bytes written: {bytesWritten}");
        }
    }

    private static nint FindMouseDeviceByVidPid(int vid, int pid, byte pingCode)
    {
        foreach (var device in Hid.Enumerate((ushort)vid, (ushort)pid))
        {
            nint deviceHandle = hid_open_path(device.Path);
            if (deviceHandle != (nint)0 && CheckPing(deviceHandle, pingCode))
            {
                return deviceHandle;
            }
            else
            {
                hid_close(deviceHandle);
            }
        }
        return (nint)0;
    }

    private static nint FindMouseDeviceByReleaseNumber(int releaseNumber, byte pingCode)
    {
        foreach (var device in Hid.Enumerate(0, 0))
        {
            if (device.ReleaseNumber == releaseNumber || device.VendorId == 1133) // by default the fw has 1133 as VID
            {
                nint deviceHandle = hid_open_path(device.Path);
                if (deviceHandle != (nint)0 && CheckPing(deviceHandle, pingCode))
                {
                    return deviceHandle;
                }
                else
                {
                    hid_close(deviceHandle);
                }
            }
        }
        return (nint)0;
    }

    private static bool CheckPing(nint deviceHandle, byte pingCode)
    {
        var pingData = new byte[] { 0, pingCode };
        hid_write(deviceHandle, pingData, 2);

        var response = new byte[64];
        int bytesRead = hid_read_timeout(deviceHandle, response, 64, 100);

        return bytesRead > 0 && response[0] == pingCode;
    }

    // Not required since we do it on the hardware side, but added it anyways
    private static int LimitXY(int xy)
    {
        return Math.Max(-32767, Math.Min(32767, xy));
    }

    private static byte LowByte(int x)
    {
        return (byte)(x & 0xFF);
    }

    private static byte HighByte(int x)
    {
        return (byte)((x >> 8) & 0xFF);
    }
}

public class DeviceNotFoundException : Exception
{
    public DeviceNotFoundException(string message) : base(message) { }
}
