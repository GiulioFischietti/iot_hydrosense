from coapthon.server.coap import CoAP
from coapthon.resources.resource import Resource

import mysql.connector
import json 

mydb = mysql.connector.connect(
        host="localhost",
        user="root",
        password="root",
        database="iot_project"
)

print('connected to db ', mydb)

class CoAPSensorDataResource(Resource):
    def __init__(self, name="CoAPSensorDataResource"):
        super(CoAPSensorDataResource, self).__init__(name)
        self.payload = "CoAP Resource"



    def render_POST(self, request):
        payload = json.loads(request.payload)
    
        json_array = payload
        sql = "INSERT INTO sensor_data (sensor_name, value) VALUES "

        for i, json_item in enumerate(json_array):
            
            sensor_name = json_item['sensor_name']
            value = json_item['value']
            
            if(i!=len(json_array)-1):
                sql += f"(\"{sensor_name}\", {value}),"
            else:
                sql += f"(\"{sensor_name}\", {value});"
	
        try:
            print("Adding sensor data into the database...")
            print(payload)
            print()
            mycursor = mydb.cursor()
            mycursor.execute(sql)
            mydb.commit()

            
            self.payload = json.dumps({"message": "Data adding successful"})
        except:
            self.payload = json.dumps({"message": "error during DATA ADDING"})
            print(f"ERROR DURING DATA ADDING")
        
        return self


class CoAPSensorListResource(Resource):
    def __init__(self, name="CoAPSensorListResource"):
        super(CoAPSensorListResource, self).__init__(name)
        self.payload = "CoAP Resource"

    
    def render_GET(self, request):
        query = request.uri_query
        params = {}
        

        if query:
            pairs = query.split('&')
            for pair in pairs:
                key, value = pair.split('=')
                params[key] = value
	
        print(f"RETRIEVING IP OF SENSOR {params['sensor_name']}")
        mycursor = mydb.cursor()
        mycursor.execute("SELECT ip FROM sensors WHERE sensor_name = \"" + params["sensor_name"] + "\";")
        myresult = mycursor.fetchall()
        
        self.payload = "{\"ip\": " + '"' + myresult[0][0].replace(" ", "") + '"' +  "}"
        return self
    
class CoAPRegistrationResource(Resource):
    def __init__(self, name="CoAPRegistrationResource"):
        super(CoAPRegistrationResource, self).__init__(name)
        self.payload = "CoAP Resource"

    def render_POST(self, request):
        payload = json.loads(request.payload)
        sensor_name = payload['sensor_name']
        ip = payload['ip'].replace(" ", "")
        
        print(f"REGISTRATION SENSOR: {sensor_name}...")
        try:
            mycursor = mydb.cursor()
            
            sql = "REPLACE INTO sensors VALUES (%s, %s);"
            val = (sensor_name, ip)

            mycursor.execute(sql, val)
            mydb.commit()

		
            self.payload = "{\"message\": \"data inserted\"}"
            print(f"REGISTRATION OF SENSOR {sensor_name} SUCCESSFUL\n")
	
        except:
            print(f"ERROR DURING REGISTRATION OF SENSOR {sensor_name}\n")
            self.payload = "{\"message\": \"error during registeration of sensor\"}"
            
            return self

class CoAPServer(CoAP):
    def __init__(self, host, port):
        super(CoAPServer, self).__init__((host, port))
        self.add_resource('register_sensor/', CoAPRegistrationResource())
        self.add_resource('update_sensor_data/', CoAPSensorDataResource())
        self.add_resource('sensor_ip_by_name/', CoAPSensorListResource())

def main():
    server = CoAPServer("::", 5683)
    

    try:
        server.listen(10)
    except KeyboardInterrupt:
        print("Server Shutdown")
        server.close()
        print("Exiting...")

if __name__ == "__main__":
    main()
