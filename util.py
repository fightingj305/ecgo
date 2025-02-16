import influxdb_client
import socket
from datetime import datetime
import influxdb_client, os, time
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
import numpy as np
import matplotlib.pyplot as plt

from main import Arguments
# Now you can import the module
import inference

token = os.environ.get("INFLUX_TOKEN")
org = "ECG DATA"
url = os.environ.get("INFLUX_ADDR")


client = influxdb_client.InfluxDBClient(url=url, token=token, org=org)
bucket="data"
write_api = client.write_api(write_options=SYNCHRONOUS)

query_api = client.query_api()



def write(label, id_val):
    point = influxdb_client.Point("model_output").field("label", label).field("id", np.uint(id_val))
    write_api.write(bucket=bucket, org="org", record=point)
def read():
    # Get the last 60 points from the bucket, specifically the adc_value field
    query = f'''from(bucket: "{bucket}")
  |> range(start: -1d)
  |> filter(fn: (r) => r._field == "adc_value" or r._field == "collection_id")
  |> sort(columns: ["_time"], desc: true)
  |> limit(n: 60)
  '''
    result = query_api.query(org=org, query=query)
    print(result)
    res_arr = []
    id_val = None

# Process and print the results
    for table in result:
        for record in table.records:
            # Extract fields from the records
            if record.get_field() == "adc_value":
                res_arr.append(record.get_value())
            elif record.get_field() == "collection_id" and id_val is None:
                id_val = record.get_value()
    return res_arr, id_val
def interpolate(arr):
    sampling_rate_original = 20
    num_points = len(arr)

    duration = num_points / sampling_rate_original  # duration in seconds

    # Create original timestamps
    t_original = np.linspace(0, duration, num_points, endpoint=False)

    # Desired sampling rate
    sampling_rate_new = 360  # Hz
    num_points_new = int(duration * sampling_rate_new)

    # Create new timestamps
    t_new = np.linspace(0, duration, num_points_new, endpoint=False)

    # Interpolate the data
    data_interpolated = np.interp(t_new, t_original, arr)

    print("data interpolated", data_interpolated)
    return data_interpolated
def process_data(model):
    res, id_val = read()
    interpolated_data = interpolate(res)
    # # Z score the data
    # mean = np.mean(interpolated_data)
    # std = np.std(interpolated_data)
    # z_score_data = (interpolated_data - mean) / std

    # assert len(z_score_data) == 1080
    print("z_score_data", len(interpolated_data))

    print("Performing inference")
    label = inference.forward(model, interpolated_data)
    print("writing data")
    write(label, id_val)
    print("written data")



if __name__ == "__main__":
    model = inference.load_model()
    process_data(model)

# with open(path, "a") as file:
#     file.write(CSV_HEADER+"\n")

# with open("desired_headers.txt","r") as file:
#     desired_headers = list(map(lambda x:x.strip("\n "),file.readlines()))
# print(f"Printing: {desired_headers}")
# while True:
#     dataDict = {}
#     data, addr = sock.recvfrom(2048) # buffer size is 2048 bytes

#     strdata = data.decode("utf-8")
    
#     with open(path, "a") as file:
#         file.write(strdata)
#     print("\n"*25)
#     # cleaner print if one string printed all at once, then carriage return
#     headers = CSV_HEADER.split(",")
#     datas = strdata.split(",")
#     printstr = ""
#     for i in range(len(headers)):
#         dataDict[headers[i]] = datas[i]
#         data = dataDict[headers[i]]
#         try:
#             data = float(data)
#         except:
#             pass
#         if "CellVoltages_" in headers[i]:
#             point = (
#                 Point(headers[i])
#                 .tag("cell_number",int(headers[i][13:]))
#                 .field("output_value", data)
#             )
#         elif "Thermistor_Temperature" in headers[i]:
#             point = (
#                 Point(headers[i])
#                 .tag("thermistor_number",int(headers[i][22:]))
#                 .field("output_value", data)
#             )
#         else:
#             point = (
#                 Point(headers[i])
#                 .field("output_value", data)
#             )
#         write_api.write(bucket=bucket, org="Stanford Solar Car Project", record=point)

#     for i in range(len(desired_headers)):
#         header = desired_headers[i]
#         blank = ' ' * (30 - len(header))   
#         blank2 = ' ' * (6 - len(dataDict[header]))    
#         printstr = printstr + f"{header}:{blank}{dataDict[header]}{blank2}"
#         if (i%3 == 0):
#             printstr+="\n"
#         else:
#             printstr+="\t"
#     print(printstr, end="\r")
