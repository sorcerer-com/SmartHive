using System;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;
using LiveCharts;
using LiveCharts.Wpf;
using LiveCharts.Wpf.Charts.Base;
using SmartHiveViewer.Models;

namespace SmartHiveViewer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private int minIndex;
        private int countElements;

        public MainWindow()
        {
            InitializeComponent();

            Loaded += MainWindow_LoadedAsync;
            SizeChanged += (_, __) => { if (IsLoaded) InitializeControls(); };
        }

        private async void MainWindow_LoadedAsync(object sender, RoutedEventArgs e)
        {
            versionTextBlock.Text = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();

            await Task.Run(() =>
            {
                InstallUtil.Install();
                if (!DataService.ImportData())
                {
                    MessageBox.Show("Данните не могат да бъдат разчетени!", "Cannot Import Data",
                        MessageBoxButton.OK, MessageBoxImage.Error);
                    Environment.Exit(0);
                }
                if (!DataService.ReadData())
                {
                    MessageBox.Show("Не е поставена флашка или няма данни на нея!", "No Data",
                        MessageBoxButton.OK, MessageBoxImage.Warning);
                }
                if (!DataService.ExportData())
                {
                    MessageBox.Show("Данните не могат да бъдат записани!", "No Data",
                        MessageBoxButton.OK, MessageBoxImage.Error);
                }
            });

            loadingTextBlock.Visibility = Visibility.Collapsed;
            tabControl.Visibility = Visibility.Visible;

            minIndex = DataService.Times
                .Where(t => t <= DataService.Times.Max().Subtract(TimeSpan.FromDays(7))).Count();
            countElements = DataService.Times.Count - minIndex;

            InitializeControls();

            var timer = new DispatcherTimer()
            {
                Interval = TimeSpan.FromMilliseconds(200)
            };
            timer.Tick += Timer_Tick;
            timer.Start();
        }


        private void InitializeControls()
        {
            tabControl.Items.Clear();
            var count = DataService.SensorsCount;
            for (int i = 1; i <= count; i++)
            {
                var tabItem = new TabItem()
                {
                    Header = "Кошер " + i,
                    IsSelected = i == 1,
                    Content = GetContent(i)
                };
                tabControl.Items.Add(tabItem);
            }
        }

        private FrameworkElement GetContent(int sensorId)
        {
            var stackPanel = new StackPanel { Orientation = Orientation.Vertical };

            var textBlock2 = new TextBlock()
            {
                Text = "Последно активен: " + DataService.Data.Where(d => d.SensorId == sensorId).First().LastActivity,
                HorizontalAlignment = HorizontalAlignment.Left,
                Margin = new Thickness(10, 5, 10, 10),
                FontSize = 16
            };
            stackPanel.Children.Add(textBlock2);

            var types = DataService.GetTypes(sensorId);
            foreach (var type in types)
            {
                var textBlock = new TextBlock()
                {
                    Text = Translate(type),
                    FontSize = 24,
                    HorizontalAlignment = HorizontalAlignment.Center
                };
                stackPanel.Children.Add(textBlock);

                var chart = GetChart(sensorId, type);
                stackPanel.Children.Add(chart);
            }

            return new ScrollViewer { Content = stackPanel };
        }

        private Chart GetChart(int sensorId, string type)
        {
            var values = DataService.GetData(sensorId, type, minIndex, countElements);

            var axisX = new Axis()
            {
                Labels = values.Keys.Select(k => k.ToString()).ToList(),
                MinValue = 0,
                MaxValue = countElements - 1,
                Foreground = System.Windows.Media.Brushes.Gray,
                FontSize = 14
            };
            var axisY = new Axis()
            {
                Foreground = System.Windows.Media.Brushes.Gray,
                FontSize = 14
            };

            var lineSeries = new LineSeries
            {
                Title = Translate(type),
                Values = new ChartValues<double>(values.Values)
            };

            var chart = new CartesianChart
            {
                Name = type,
                Tag = sensorId,
                Height = Math.Max(200, ActualHeight / 2.5),
                Margin = new Thickness(0, 5, 0, 25),
                Zoom = ZoomingOptions.X,
                DisableAnimations = true,
                FontSize = 14
            };
            chart.AxisX.Add(axisX);
            chart.AxisY.Add(axisY);
            chart.Series.Add(lineSeries);

            chart.MouseDoubleClick += (_, e) =>
            {
                if (e.LeftButton == MouseButtonState.Pressed) // week
                {
                    minIndex = DataService.Times
                        .Where(t => t <= DataService.Times.Max().Subtract(TimeSpan.FromDays(7))).Count();
                    countElements = DataService.Times.Count - minIndex;
                }
                else if (e.RightButton == MouseButtonState.Pressed) //day
                {
                    minIndex = DataService.Times
                        .Where(t => t <= DataService.Times.Max().Subtract(TimeSpan.FromDays(1))).Count();
                    countElements = DataService.Times.Count - minIndex;
                }
                else
                {
                    minIndex = 0;
                    countElements = DataService.Times.Count;
                }
                chart.AxisX[0].MinValue = 0;
                chart.AxisX[0].MaxValue = countElements;
            };

            return chart;
        }

        private void Timer_Tick(object sender, EventArgs e)
        {
            var scrollViewer = tabControl.SelectedContent as ScrollViewer;
            var stackPanel = scrollViewer?.Content as StackPanel;
            if (stackPanel == null)
                return;

            var groupMinValue = stackPanel.Children.OfType<Chart>().GroupBy(c => c.AxisX.First().MinValue);
            var groupMaxValue = stackPanel.Children.OfType<Chart>().GroupBy(c => c.AxisX.First().MaxValue);
            if (groupMinValue.Count() == 1 && groupMaxValue.Count() == 1)
                return;

            var chart = groupMaxValue.Where(g => g.Count() == 1).Select(g => g.First()).First();
            if ((int)chart.AxisX.First().MinValue == 0 &&
                (int)chart.AxisX.First().MaxValue == countElements - 1)
                return;

            if (Math.Sign(chart.AxisX.First().MinValue) ==
                Math.Sign(chart.AxisX.First().MaxValue - countElements - 1))
            {
                chart.AxisX.First().MaxValue = countElements - 1;
            }
            else
                countElements = (int)chart.AxisX.First().MaxValue + 1;

            minIndex = minIndex + (int)chart.AxisX.First().MinValue;
            chart.AxisX.First().MinValue -= Math.Truncate(chart.AxisX.First().MinValue);

            foreach (var child in stackPanel.Children)
            {
                if (!(child is Chart chart2))
                    continue;

                var sensorId = (int)chart2.Tag;
                var type = chart2.Name;
                var values = DataService.GetData(sensorId, type, minIndex, countElements);
                chart2.AxisX.First().Labels = values.Keys.Select(k => k.ToString()).ToList();
                chart2.Series.First().Values.Clear();
                chart2.Series.First().Values.AddRange(values.Values.OfType<object>());

                chart2.AxisX.First().MinValue = chart.AxisX.First().MinValue;
                chart2.AxisX.First().MaxValue = chart.AxisX.First().MaxValue;
            }
        }


        private static string Translate(string text)
        {
            switch (text)
            {
                case "Temperature":
                    return "Температура";
                case "Humidity":
                    return "Влажност";
                case "Weight":
                    return "Тегло";
                case "Voltage":
                    return "Волтаж";
                default:
                    return text;
            }
        }
    }
}
