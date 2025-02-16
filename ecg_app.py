from dotenv import load_dotenv
import streamlit as st
from influxdb_client import InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS
import pandas as pd
import os

load_dotenv()

# InfluxDB connection parameters
url = "https://us-east-1-1.aws.cloud2.influxdata.com"
token = os.environ["INFLUX_TOKEN"]
org = "ECG Data"
bucket = "data"

# Check if the token is available
if not token:
    st.error("INFLUX_TOKEN environment variable is not set. Please set it and restart the app.")
    st.stop()

def query_influxdb_label(sample_id):
    client = InfluxDBClient(url=url, token=token, org=org)
    query_api = client.query_api()

    query = f'''
    from(bucket: "{bucket}")
    |> range(start: -1d)
    |> filter(fn: (r) => r._field == "id" or r._field == "label")
    |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
    |> filter(fn: (r) => r.id == {sample_id})
    |> sort(columns: ["_time"])
    |> limit(n:1)
    '''

    result = query_api.query(query)

    if result and len(result) > 0 and len(result[0].records) > 0:
        return result[0].records[0].values.get('label')
    else:
        return None

def query_influxdb_ecg_data(sample_id):
    client = InfluxDBClient(url=url, token=token, org=org)
    query_api = client.query_api()

    query = f'''
    from(bucket: "{bucket}")
    |> range(start: -30d)
    |> filter(fn: (r) => r._field == "collection_id" or r._field == "adc_value" or r._field == "collection_time")
    |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
    |> filter(fn: (r) => r.collection_id == {sample_id})
    |> sort(columns: ["_time"])
    '''
    
    result = query_api.query_data_frame(query)
    
    return result

def main():
    st.title("ECG Data Lookup and Visualization App")

    st.write("Please enter a sample ID between 0 and 10000.")

    sample_id = st.number_input("Sample ID", min_value=0, max_value=10000, value=0, step=1)

    if st.button("Look up Data"):
        label = query_influxdb_label(sample_id)
        if label is not None:
            st.success(f"The label for Sample ID {sample_id} is: {label}")
        else:
            st.error(f"No label found for Sample ID {sample_id}")

        # Query ECG data
        ecg_data = query_influxdb_ecg_data(sample_id)
        
        if not ecg_data.empty:
            # Set the collection_time as the index
            ecg_data.set_index('collection_time', inplace=True)
            
            # Create the plot
            st.line_chart(ecg_data['adc_value'])
            st.write("ECG Data (ADC Value vs Collection Time)")
        else:
            st.error(f"No ECG data found for Sample ID {sample_id}")

if __name__ == "__main__":
    main()