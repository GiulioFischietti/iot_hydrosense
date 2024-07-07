import tkinter as tk
import json
import asyncio
import threading
from aiocoap import *

host = 'fd00::1'

root = tk.Tk()
root.title("HydroSense")

cornice_sensori = tk.Frame(root)
cornice_sensori.pack(pady=10)

sensore1_var = tk.StringVar()
sensore2_var = tk.StringVar()
sensore3_var = tk.StringVar()
sensore4_var = tk.StringVar()

main_pump_var = tk.StringVar(root, value="Main Pump OK")
backup_pump_var = tk.StringVar(root, value="Backup Pump OFF")

main_pump_var_color = tk.StringVar(root, value="green")

def update_backup_status(is_active):
    if(is_active==0):
        backup_pump_var.set("Backup pump OFF")
    else:
        backup_pump_var.set("Backup pump ON")

async def observe_backup_pump_status(sensorIp):
	print(f"Starting observation on {sensorIp}")
	protocol = await Context.create_client_context()
	uri = f'coap://[{sensorIp}]:5683/backup_actuator'
	request = Message(code=GET, uri=uri, observe=0)

	print(f'Sending observation request to {uri}')
	requester = protocol.request(request)

	async for response in requester.observation:
		jsonObj = json.loads(response.payload.decode())
		
		update_backup_status(jsonObj["state"])

def update_main_status(is_damaged):
    if(is_damaged==1):
        main_pump_var.set("Main Pump MAY BREAK")
        main_pump_var_color.set("red")
    else:
        main_pump_var.set("Main Pump OK")
        main_pump_var_color.set("green")

async def observe_main_pump_status(sensorIp):
	print(f"Starting observation on {sensorIp}")
	protocol = await Context.create_client_context()
	uri = f'coap://[{sensorIp}]:5683/main_pump_state'
	request = Message(code=GET, uri=uri, observe=0)

	print(f'Sending observation request to {uri}')
	requester = protocol.request(request)

	async for response in requester.observation:
		jsonObj = json.loads(response.payload.decode())
		print(jsonObj)
		update_main_status(jsonObj["state"])

async def observe_vibration_sensor_data(sensorIp):
	print(f"Starting observation on {sensorIp}")
	protocol = await Context.create_client_context()
	uri = f'coap://[{sensorIp}]:5683/vibration/push'
	request = Message(code=GET, uri=uri, observe=0)

	print(f'Sending observation request to {uri}')
	requester = protocol.request(request)

	async for response in requester.observation:
		jsonObj = json.loads(response.payload.decode())
		sensore3_var.set("Vibration: " + str(jsonObj["value"]))
		

async def observe_temperature_sensor_data(sensorIp):
	print(f"Starting observation on {sensorIp}")
	protocol = await Context.create_client_context()
	uri = f'coap://[{sensorIp}]:5683/temp/push'
	request = Message(code=GET, uri=uri, observe=0)

	print(f'Sending observation request to {uri}')
	requester = protocol.request(request)

	async for response in requester.observation:
		jsonObj = json.loads(response.payload.decode())
		sensore2_var.set("Temperature: " + str(jsonObj["value"]))
		

async def observe_pressure_sensor_data(sensorIp):
	print(f"Starting observation on {sensorIp}")
	protocol = await Context.create_client_context()
	uri = f'coap://[{sensorIp}]:5683/pressure/push'
	request = Message(code=GET, uri=uri, observe=0)

	print(f'Sending observation request to {uri}')
	requester = protocol.request(request)

	async for response in requester.observation:
		jsonObj = json.loads(response.payload.decode())
		sensore4_var.set("Pressure: " + str(jsonObj["value"]))
		

async def observe_flow_sensor_data(sensorIp):
	print(f"Starting observation on {sensorIp}")
	protocol = await Context.create_client_context()
	uri = f'coap://[{sensorIp}]:5683/flow'
	request = Message(code=GET, uri=uri, observe=0)

	print(f'Sending observation request to {uri}')
	requester = protocol.request(request)

	async for response in requester.observation:
		jsonObj = json.loads(response.payload.decode())
		sensore1_var.set("Flow: " + str(jsonObj["value"]))

async def activate_backup_pump():
	protocol = await Context.create_client_context()
	uri = f'coap://[{backupActuatorIp}]:5683/backup_actuator'
	payload = {"action": "on"}

	payload_json = json.dumps(payload).encode('utf-8')
	
	request = Message(code=PUT, uri=uri, payload=payload_json)
	request.opt.content_format = 50
	
	
	response = await protocol.request(request).response

	response_text = response.payload.decode('utf-8')
	print(f'Response: {response_text}')
	
	return None


async def set_normal_mode():
	protocol = await Context.create_client_context()
	uri = f'coap://[{sensorFlowIp}]:5683/main_pump_state'
	payload = {"status": 0}
	payload_json = json.dumps(payload).encode('utf-8')
	request = Message(code=PUT, uri=uri, payload=payload_json)	
	
	response = await protocol.request(request).response
	response_text = response.payload.decode('utf-8')
	print(f'Response: {response_text}')
	
	return None
	
	
async def set_recovery_mode():
	protocol = await Context.create_client_context()
	uri = f'coap://[{sensorFlowIp}]:5683/main_pump_state'
	payload = {"status": 1}

	payload_json = json.dumps(payload).encode('utf-8')
	
	request = Message(code=PUT, uri=uri, payload=payload_json)
	request.opt.content_format = 50
	
	
	response = await protocol.request(request).response

	response_text = response.payload.decode('utf-8')
	print(f'Response: {response_text}')
	
	return None


async def deactivate_backup_pump():
	protocol = await Context.create_client_context()
	uri = f'coap://[{backupActuatorIp}]:5683/backup_actuator'
	payload = {"action": "off"}

	payload_json = json.dumps(payload).encode('utf-8')
	
	request = Message(code=PUT, uri=uri, payload=payload_json)
	request.opt.content_format = 50
	
	
	response = await protocol.request(request).response

	response_text = response.payload.decode('utf-8')
	print(f'Response: {response_text}')
	
	return None

async def get_sensor_ip(sensorName):
	protocol = await Context.create_client_context()
	uri = f'coap://[{host}]:5683/sensor_ip_by_name?sensor_name={sensorName}'
	request = Message(code=GET, uri=uri)

	print(f'Sending request to {sensorName}')
	response = await protocol.request(request).response

	response_text = response.payload.decode('utf-8')
	print(f'Response: {response_text}')
	
	return json.loads(response_text)["ip"]

def start_asyncio_loop():
	loop = asyncio.new_event_loop()
	asyncio.set_event_loop(loop)
	
	global sensorFlowIp
	sensorFlowIp = loop.run_until_complete(get_sensor_ip('flow'))
	sensorFlowIp = sensorFlowIp.replace("fe80", "fd00")
	
	global sensorTempIp
	sensorTempIp = loop.run_until_complete(get_sensor_ip('temp'))
	sensorTempIp = sensorTempIp.replace("fe80", "fd00")
	
	global sensorVibrationIp
	sensorVibrationIp = loop.run_until_complete(get_sensor_ip('vibration'))
	sensorVibrationIp = sensorVibrationIp.replace("fe80", "fd00")
	
	global sensorPressureIp
	sensorPressureIp = loop.run_until_complete(get_sensor_ip('pressure'))
	sensorPressureIp = sensorPressureIp.replace("fe80", "fd00")
	
	global backupActuatorIp
	backupActuatorIp = loop.run_until_complete(get_sensor_ip('backup_actuator'))
	backupActuatorIp = backupActuatorIp.replace("fe80", "fd00")
	
	loop2 = asyncio.new_event_loop()
	loop2.create_task(observe_flow_sensor_data(sensorFlowIp))
	loop2.create_task(observe_vibration_sensor_data(sensorVibrationIp))
	loop2.create_task(observe_pressure_sensor_data(sensorPressureIp))
	loop2.create_task(observe_temperature_sensor_data(sensorTempIp))
	loop2.create_task(observe_main_pump_status(sensorFlowIp))
	loop2.create_task(observe_backup_pump_status(backupActuatorIp))
	
	print("running forever")
	loop2.run_forever()

def createGUI(loop):
	sensore1_var.set("Flow: 0")
	sensore2_var.set("Temperature: 0")
	sensore3_var.set("Vibration: 0")
	sensore4_var.set("Pressure: 0")
	
	sensore1_label = tk.Label(cornice_sensori, textvariable=sensore1_var)
	sensore2_label = tk.Label(cornice_sensori, textvariable=sensore2_var)
	sensore3_label = tk.Label(cornice_sensori, textvariable=sensore3_var)
	sensore4_label = tk.Label(cornice_sensori, textvariable=sensore4_var)

	sensore1_label.grid(row=0, column=0, padx=10, pady=5)
	sensore2_label.grid(row=0, column=1, padx=10, pady=5)
	sensore3_label.grid(row=1, column=0, padx=10, pady=5)
	sensore4_label.grid(row=1, column=1, padx=10, pady=5)

	cornice_pulsanti = tk.Frame(root)
	cornice_pulsanti.pack(side=tk.BOTTOM, pady=10)

	pulsante1 = tk.Button(cornice_pulsanti, text="Switch On Backup Pump", command=lambda: asyncio.run(activate_backup_pump()))
	pulsante2 = tk.Button(cornice_pulsanti, text="Switch Off Backup Pump", command=lambda: asyncio.run(deactivate_backup_pump()))
	pulsante3 = tk.Button(cornice_pulsanti, text="Set NORMAL mode", command=lambda: asyncio.run(set_normal_mode()))
	pulsante4 = tk.Button(cornice_pulsanti, text="Set Recovery Mode", command=lambda: asyncio.run(set_recovery_mode()))
	
	
	pulsante1.pack(side=tk.LEFT, padx=5)
	pulsante2.pack(side=tk.LEFT, padx=5)
	pulsante3.pack(side=tk.LEFT, padx=5)
	pulsante4.pack(side=tk.LEFT, padx=5)
	
	main_status_label = tk.Label(root, textvariable=main_pump_var, fg="black", font=("Helvetica", 16))
	main_status_label.pack(pady=20)
	
	backup_status_label = tk.Label(root, textvariable=backup_pump_var, fg="black", font=("Helvetica", 12))
	backup_status_label.pack(pady=20)
	root.mainloop()

	
if __name__ == "__main__":
	asyncio_thread = threading.Thread(target=start_asyncio_loop)
	asyncio_thread.start()

	loop = asyncio.new_event_loop()
	asyncio.set_event_loop(loop)
	createGUI(loop)
	
	
	
	
	
	
	
	
