using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace SmartHiveViewer.Models
{
    public static class DataService
    {
        public static List<DataClass> Data = new List<DataClass>();
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


        public static int SensorsCount => Data.GroupBy(d => d.SensorId).Count();

        public static List<string> GetTypes(int sensorId)
        {
            return Data
                .Where(d => d.SensorId == sensorId)
                .GroupBy(d => d.Type)
                .Select(g => g.Key)
                .OrderBy(d => d)
                .ToList();
        }

        public static SortedDictionary<DateTime, double> GetData(int sensorId, string type)
        {
            var result = Data
                .Where(d => d.SensorId == sensorId && d.Type.ToLower() == type.ToLower())
                .OrderBy(d => d.Timestamp)
                .Distinct()
                .ToDictionary(d => d.Timestamp, d => d.Value);

            var times = Data.Select(d => d.Timestamp).Distinct().OrderBy(d => d).ToList();
            foreach (var time in times)
            {
                if (!result.ContainsKey(time))
                {
                    var prevValue = result
                        .Where(kvp => kvp.Key < time)
                        .OrderBy(kvp => kvp.Key)
                        .Select(kvp => kvp.Value);

                    if (prevValue.Count() > 0)
                        result.Add(time, prevValue.Last());
                    else
                        result.Add(time, 0);
                }
            }

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
