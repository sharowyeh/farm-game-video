using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace topmost_video_playback
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

        [DllImportAttribute("user32.dll")]
        public static extern bool ReleaseCapture();

        //Attach this to the MouseDown event of your drag control to move the window in place of the title bar
        private void WindowDrag(object sender, MouseButtonEventArgs e) // MouseDown
        {
            ReleaseCapture();
            SendMessage(new WindowInteropHelper(this).Handle,
                0xA1, (IntPtr)0x2, (IntPtr)0);
        }

        //Attach this to the PreviewMousLeftButtonDown event of the grip control in the lower right corner of the form to resize the window
        private void WindowResize(object sender, MouseButtonEventArgs e) //PreviewMousLeftButtonDown
        {
            HwndSource hwndSource = PresentationSource.FromVisual((Visual)sender) as HwndSource;
            SendMessage(hwndSource.Handle, 0x112, (IntPtr)61448, IntPtr.Zero);
        }

        public MainWindow()
        {
            InitializeComponent();
            this.PreviewMouseLeftButtonDown += (sender, e) =>
            {
                var pt = e.GetPosition(this);
                // Top caption and avoid buttons
                if (pt.Y < 30 && pt.X > 50 && this.Width - pt.X > 100)
                {
                    WindowDrag(sender, e);
                }
                // Window right and bottom corner, but WebBrowser cannot receives any wpf events
                else if (this.Width - pt.X < 10 || this.Height - pt.Y < 10)
                {
                    WindowResize(sender, e);
                }
            };
            UrlTextBox.KeyDown += (sender, e) =>
            {
                if (e.Key == Key.Enter && !string.IsNullOrEmpty(UrlTextBox.Text))
                {
                    RenderBrowser.Load(UrlTextBox.Text);
                }
            };
            GoButton.Click += (sender, e) =>
            {
                if (!string.IsNullOrEmpty(UrlTextBox.Text))
                {
                    RenderBrowser.Load(UrlTextBox.Text);
                }
            };
            CloseButton.Click += (sender, e) =>
            {
                this.Close();
            };
        }
    }
}
