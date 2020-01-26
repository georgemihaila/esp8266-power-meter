using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using PowerMeter.Models;

namespace PowerMeter.Controllers
{
    public class APIController : Controller
    {
        private readonly ApplicationDbContext _context;
        private static DateTime _lastReadingTime = DateTime.Now;

        public APIController(ApplicationDbContext context)
        {
            _context = context;
        }

        [HttpPost]
        public IActionResult AddReading()
        {
            var now = DateTime.Now;
            var consumption = 3600 / (now.Subtract(_lastReadingTime).TotalSeconds);
            _lastReadingTime = now;
            if (Math.Abs(consumption) > 5000) //Ignore readings that are too high.
            {
                Console.WriteLine($"Invalid reading ({consumption})");
                return Ok();
            }
            var watts = (consumption / 1000).ToString("N3");
            var reading = new
            {
                timestamp = now,
                electricity_delivered_1 = 0,
                electricity_returned_1 = 0,
                electricity_delivered_2 = IncrementAndGetTotalEnergyUsage_kWh().ToString("F3"),
                electricity_returned_2 = 0,
                electricity_currently_delivered = watts,
                electricity_currently_returned = 0,
                phase_currently_delivered_l1 = watts
            };
            var request = (WebRequest)HttpWebRequest.Create("http://10.2.0.5/api/v2/datalogger/dsmrreading");
            request.Method = "POST";
            request.Headers.Add("X-AUTHKEY", "YOUR-DSMR-READER-API-KEY");                   
            request.Headers[HttpRequestHeader.ContentType] = "application/json";
            var stream = request.GetRequestStream();
            using (var writer = new StreamWriter(stream))
            {
                writer.Write(JsonConvert.SerializeObject(reading));
            }
            var response = request.GetResponse();

            IncrementTodaysEnergyUsage_kWh();

            return Ok();
        }

            private static double IncrementAndGetTotalEnergyUsage_kWh()
            {
            var filename = "cache/total.dat";
            double result = 0;
            if (System.IO.File.Exists(filename))
            {
                result = Convert.ToDouble(System.IO.File.ReadAllText(filename)) + 0.001; // + 1Wh per reading
            }
            System.IO.File.WriteAllText(filename, result.ToString());
                return result;
            }


        private static void IncrementTodaysEnergyUsage_kWh()
        {
            var filename = string.Format("cache/{0}_{1}_{2}.dat", DateTime.Now.Year, DateTime.Now.Month, DateTime.Now.Day);
                double result = 0;
                if (System.IO.File.Exists(filename))
                {
                    result = Convert.ToDouble(System.IO.File.ReadAllText(filename)) + 0.001; // + 1Wh per reading
                System.IO.File.WriteAllText(filename, result.ToString());
        }

        [ResponseCache(Duration = 0, Location = ResponseCacheLocation.None, NoStore = true)]
        public IActionResult Error()
        {
            return View(new ErrorViewModel { RequestId = Activity.Current?.Id ?? HttpContext.TraceIdentifier });               
    }
    }
}