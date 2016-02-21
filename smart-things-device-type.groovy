/**
 * Particle (Spark) Core / Photon / Electron Remote Temperature and Humidity Logger
 *
 * Author: Nic Jansma
 *
 * Licensed under the MIT license
 *
 * Available at: https://github.com/nicjansma/dht-logger/
 */

preferences {
    input name: "deviceId", type: "text", title: "Device ID", required: true
    input name: "token", type: "password", title: "Access Token", required: true
    input name: "sparkTemperatureVar", type: "text", title: "Spark Temperature Variable", required: true, defaultValue: "temperature"
    input name: "sparkHumidityVar", type: "text", title: "Spark Humidity Variable", required: true, defaultValue: "humidity"
    input name: "sparkHeatIndexVar", type: "text", title: "Spark Heat Index Variable", required: true, defaultValue: "heatIndex"
}

metadata {
    definition (name: "Particle Remote Temperature and Humidity Logger", namespace: "nicjansma", author: "Nic Jansma") {
        capability "Polling"
        capability "Sensor"
        capability "Refresh"
        capability "Relative Humidity Measurement"
        capability "Temperature Measurement"

        attribute "temperature", "number"
    }

    tiles(scale: 2) {
        valueTile("temperature", "device.temperature", width: 2, height: 2) {
            state("temperature", label:'${currentValue}°', unit:"F",
                backgroundColors:[    
                    [value: 31, color: "#153591"],
                    [value: 44, color: "#1e9cbb"],
                    [value: 59, color: "#90d2a7"],
                    [value: 74, color: "#44b621"],
                    [value: 84, color: "#f1d801"],
                    [value: 95, color: "#d04e00"],
                    [value: 96, color: "#bc2323"]
                ]
            )
        }

        valueTile("heatIndex", "device.heatIndex", width: 2, height: 2) {
            state("heatIndex", label:'${currentValue}°', unit:"F",
                backgroundColors:[    
                    [value: 31, color: "#153591"],
                    [value: 44, color: "#1e9cbb"],
                    [value: 59, color: "#90d2a7"],
                    [value: 74, color: "#44b621"],
                    [value: 84, color: "#f1d801"],
                    [value: 95, color: "#d04e00"],
                    [value: 96, color: "#bc2323"]
                ]
            )
        }

        valueTile("humidity", "device.humidity", width: 2, height: 2) {
            state "default", label:'${currentValue}%'
        }

        standardTile("refresh", "device.refresh", inactiveLabel: false, decoration: "flat", width: 2, height: 2) {
            state "default", action:"refresh.refresh", icon:"st.secondary.refresh"
        }

        main "temperature"
        details(["temperature", "humidity", "heatIndex", "refresh"])
    }
}

// handle commands
def poll() {
    log.debug "Executing 'poll'"

    getAll()
}

def refresh() {
    log.debug "Executing 'refresh'"

    getAll()
}

def getAll() {
    getTemperature()
    getHumidity()
    getHeatIndex()
}

def parse(String description) {
    def pair = description.split(":")

    createEvent(name: pair[0].trim(), value: pair[1].trim())
}

private getTemperature() {
    def closure = { response ->
        log.debug "Temperature request was successful, $response.data"

        sendEvent(name: "temperature", value: response.data.result)
    }

    httpGet("https://api.spark.io/v1/devices/${deviceId}/${sparkTemperatureVar}?access_token=${token}", closure)
}

private getHumidity() {
    def closure = { response ->
        log.debug "Humidity request was successful, $response.data"

        sendEvent(name: "humidity", value: response.data.result)
    }

    httpGet("https://api.spark.io/v1/devices/${deviceId}/${sparkHumidityVar}?access_token=${token}", closure)
}

private getHeatIndex() {
    def closure = { response ->
        log.debug "Heat Index request was successful, $response.data"

        sendEvent(name: "heatIndex", value: response.data.result)
    }

    httpGet("https://api.spark.io/v1/devices/${deviceId}/${sparkHeatIndexVar}?access_token=${token}", closure)
}
