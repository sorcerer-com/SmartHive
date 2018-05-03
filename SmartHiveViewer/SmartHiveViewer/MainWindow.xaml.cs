using System;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
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

            InitializeControls();
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
                FontSize = 14
            };
            stackPanel.Children.Add(textBlock2);

            var types = DataService.GetTypes(sensorId);
            foreach (var type in types)
            {
                var textBlock = new TextBlock()
                {
                    Text = Translate(type),
                    HorizontalAlignment = HorizontalAlignment.Center
                };
                stackPanel.Children.Add(textBlock);

                var chart = GetChart(sensorId, type);
                stackPanel.Children.Add(chart);


                // auto scale all charts
                EventHandler handler = (_, __) =>
                {
                    foreach (var child in stackPanel.Children)
                    {
                        if (child is Chart chart2 && chart != chart2)
                        {
                            if (Math.Abs(chart2.AxisX[0].MinValue - chart.AxisX[0].MinValue) > 1)
                            {
                                chart2.AxisX[0].MinValue = chart.AxisX[0].MinValue;
                            }
                            if (double.IsNaN(chart.AxisX[0].MaxValue) || double.IsNaN(chart2.AxisX[0].MaxValue) ||
                                Math.Abs(chart2.AxisX[0].MaxValue - chart.AxisX[0].MaxValue) > 1)
                            {
                                chart2.AxisX[0].MaxValue = chart.AxisX[0].MaxValue;
                            }
                        }
                    }
                };
                System.ComponentModel.DependencyPropertyDescriptor
                    .FromProperty(Axis.MinValueProperty, typeof(Axis))
                    .AddValueChanged(chart.AxisX[0], handler);
                System.ComponentModel.DependencyPropertyDescriptor
                    .FromProperty(Axis.MaxValueProperty, typeof(Axis))
                    .AddValueChanged(chart.AxisX[0], handler);
            }

            return new ScrollViewer { Content = stackPanel };
        }

        private Chart GetChart(int sensorId, string type)
        {
            var values = DataService.GetData(sensorId, type);
            var dayValuesCount = values
                .Where(v => v.Key < values.Keys.Max().Subtract(TimeSpan.FromDays(1))).Count();
            var weekValuesCount = values
                .Where(v => v.Key < values.Keys.Max().Subtract(TimeSpan.FromDays(7))).Count();

            var axisX = new Axis()
            {
                Labels = values.Keys.OrderBy(k => k).Select(k => k.ToString()).ToList(),
                MinValue = dayValuesCount
            };

            var lineSeries = new LineSeries
            {
                Title = Translate(type),
                Values = new ChartValues<double>(values.Values)
            };

            var chart = new CartesianChart
            {
                Height = Math.Max(200, ActualHeight / 2.5),
                Margin = new Thickness(0, 5, 0, 25),
                Zoom = ZoomingOptions.X,
                DisableAnimations = true
            };
            chart.AxisX.Add(axisX);
            chart.Series.Add(lineSeries);

            chart.MouseDoubleClick += (_, e) =>
            {
                if (e.LeftButton == MouseButtonState.Pressed)
                    chart.AxisX[0].MinValue = weekValuesCount;
                else if (e.RightButton == MouseButtonState.Pressed)
                    chart.AxisX[0].MinValue = dayValuesCount;
                else
                    chart.AxisX[0].MinValue = double.NaN;
                chart.AxisX[0].MaxValue = double.NaN;
            };

            return chart;
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
