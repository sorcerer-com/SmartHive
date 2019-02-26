using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace SmartHiveViewer.Models
{
    public static class DataService
    {
        private static List<DataClass> Data = new List<DataClass>();
        private static bool newData = false;

        public static bool ReadData()
        {
            bool readSuccessful = false;

            var dirs = InstallUtil.GetRemovableDrives();
            foreach (var dir in dirs)
            {
                var csvFilePath = dir.GetFiles("data.csv").SingleOrDefault();
                if (csvFilePath == null)
                    continue;

                var content = ReadFile(csvFilePath.FullName);
                content = content.Except(Data).ToList();
                if (content.Count > 0)
                {
                    Data.AddRange(content);
                    newData = true;
                }
                readSuccessful = true;
            }
            Data.Sort((a, b) => b.Timestamp.CompareTo(a.Timestamp));
            return readSuccessful;
        }

        public static bool ImportData()
        {
            try
            {
                var filePath = Path.Combine(Environment.GetFolderPath(
                    Environment.SpecialFolder.ApplicationData), "SmartHive", "SmartHiveData.csv");

                if (!File.Exists(filePath))
                    return true;

                var content = ReadFile(filePath);
                Data.AddRange(content);
            }
            catch
            {
                return false;
            }
            return true;
        }

        public static bool ExportData()
        {
            if (!newData)
                return true;
            try
            {
                var filePath = Path.Combine(Environment.GetFolderPath(
                    Environment.SpecialFolder.ApplicationData), "SmartHive", "SmartHiveData.csv");

                if (File.Exists(filePath))
                    File.Copy(filePath, filePath + ".bak", true);

                var content = new List<string>();
                foreach (var data in Data)
                    content.Add(data.ToCSVLine());
                File.WriteAllLines(filePath, content);
            }
            catch
            {
                return false;
            }
            return true;
        }


        private static DateTime periodStart = DateTime.Now.AddMonths(-3);
        public static DateTime PeriodStart
        {
            get => periodStart;
            set
            {
                if (periodStart == value)
                    return;

                periodStart = value;
                times = null;
            }
        }

        public static int SensorsCount => Data.GroupBy(d => d.SensorId).Count();

        private static List<DateTime> times = null;
        public static List<DateTime> Times
        {
            get
            {
                if (times == null)
                {
                    times = Data
                        .Where(d => d.Timestamp > PeriodStart)
                        .Select(d => d.Timestamp)
                        .Distinct()
                        .OrderBy(t => t)
                        .ToList();
                }
                return times;
            }
        }

        public static List<string> GetTypes(int sensorId)
        {
            return Data
                .Where(d => d.SensorId == sensorId)
                .GroupBy(d => d.Type)
                .Select(g => g.Key)
                .OrderBy(d => d)
                .ToList();
        }

        public static DateTime GetLastActivity(int sensorId)
        {
            return Data
                .Where(d => d.SensorId == sensorId)
                .First()
                .LastActivity;
        }

        private static Dictionary<string, Dictionary<DateTime, double>> cache = 
            new Dictionary<string, Dictionary<DateTime, double>>();

        public static SortedDictionary<DateTime, double> GetData(int sensorId, string type,
            int minIndex = 0, int count = int.MaxValue)
        {
            var id = $"{sensorId}.{type}";
            if (!cache.TryGetValue(id, out var result))
            {
                result = Data
                    .Where(d => d.SensorId == sensorId && d.Type.ToLower() == type.ToLower())
                    .GroupBy(d => d.Timestamp)
                    .Select(g => g.First())
                    .ToDictionary(d => d.Timestamp, d => d.Value);

                // fill gaps between values
                var keys = result.Keys.OrderBy(t => t).ToList();
                int prevKeyIdx = 0;
                var times = Data.Select(d => d.Timestamp).Distinct().OrderBy(t => t);
                foreach (var time in times)
                {
                    if (result.ContainsKey(time))
                        continue;
                    
                    while (prevKeyIdx < keys.Count - 1 && keys[prevKeyIdx] < time)
                        prevKeyIdx++;

                    if (prevKeyIdx > 0)
                        result.Add(time, result[keys[prevKeyIdx - 1]]);
                    else
                        result.Add(time, 0);
                }
                cache.Add(id, result);
            }
            result = result.Where(kvp => kvp.Key > PeriodStart).OrderBy(kvp => kvp.Key).Skip(minIndex).Take(count)
                .ToDictionary(kvp => kvp.Key, kvp => kvp.Value);

            return new SortedDictionary<DateTime, double>(result);
        }


        private static List<DataClass> ReadFile(string filePath)
        {
            try
            {
                var result = new List<DataClass>();

                var lines = File.ReadLines(filePath);
                foreach (var line in lines)
                {
                    var data = DataClass.Parse(line);
                    if (data != null)
                        result.Add(data);
                }
                return result;
            }
            catch
            {
                return new List<DataClass>();
            }
        }
    }
}
