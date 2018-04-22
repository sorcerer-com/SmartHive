using System;
using System.Globalization;

namespace SmartHiveViewer.Models
{
    public class DataClass
    {
        public DateTime Timestamp { get; set; }
        public int SensorId { get; set; }
        public string SensorMAC { get; set; }
        public string Type { get; set; }
        public double Value { get; set; }
        public DateTime LastActivity { get; set; }


        public DataClass(DateTime timestamp, int sensorId, string sensorMAC, 
            string type, double value, DateTime lastActivity)
        {
            Timestamp = timestamp;
            SensorId = sensorId;
            SensorMAC = sensorMAC;
            Type = type;
            Value = value;
            LastActivity = lastActivity;
        }


        public string ToCSVLine()
        {
            return $"{Timestamp},{SensorId},{SensorMAC},{Type},{Value.ToString(CultureInfo.InvariantCulture)},{LastActivity}";
        }


        public override bool Equals(object obj)
        {
            return GetHashCode() == obj.GetHashCode();
        }

        public override int GetHashCode()
        {
            return 
                Timestamp.GetHashCode() + 
                SensorId.GetHashCode() + 
                SensorMAC.GetHashCode() + 
                Type.GetHashCode() + 
                Value.GetHashCode();
        }

        public override string ToString()
        {
            return $"{Timestamp} ({SensorId}) {Type}: {Value}";
        }


        public static DataClass Parse(string line)
        {
            var split = line.Split(',');
            if (split.Length != 6)
                return null;

            if (!DateTime.TryParse(split[0], out DateTime timestamp))
                return null;

            if (!int.TryParse(split[1], out int sensorId))
                return null;

            string sensorMAC = split[2];
            string type = split[3];

            if (!double.TryParse(split[4], NumberStyles.Float, CultureInfo.InvariantCulture, 
                out double value))
                return null;

            if (!DateTime.TryParse(split[5], out DateTime lastActivity))
                return null;

            return new DataClass(timestamp, sensorId, sensorMAC, type, value, lastActivity);
        }
    }
}
