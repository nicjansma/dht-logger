//
// Creates a DynamoDB row with your IoT data.
// By Nic Jansma http://nicj.net
//
// https://github.com/nicjansma/dht-logger/
//
// Based on: https://snowulf.com/2015/08/05/tutorial-aws-api-gateway-to-lambda-to-dynamodb/
//

//
// Imports
//
var doc = require("dynamodb-doc");

//
// Constants
//
var TABLE_NAME = "iot";

var dynamodb = new doc.DynamoDB();

exports.handler = function(event, context) {
    console.log("Request received:\n", JSON.stringify(event));
    console.log("Context received:\n", JSON.stringify(context));

    var dateTime = new Date().getTime();

    var item = {
        "device": event.device, 
        "time": dateTime
    };

    if (typeof event.coreid !== "undefined") {
        item.deviceId = event.coreid;
    }

    if (typeof event.event !== "undefined") {
        item.event = event.event;
    }

    // parse data payload
    var data;

    // if there is a 'data' property, it's possibly from a Particle Webhook, and we should use those values
    if (typeof event.data !== "undefined" && typeof event.published_at !== "undefined") {
        try {
            data = JSON.parse(event.data);
        } catch (e) {
            return context.fail("Failed to parse data JSON");
        }
    } else {
        data = event;
    }

    // copy data properties over to DynamoDB item
    for (var key in data) {
        if (data.hasOwnProperty(key) &&
            typeof data[key] !== "undefined" &&
            data[key] !== null &&
            data[key] !== "") {
            item[key] = data[key];
        }
    }

    console.log("Item:\n", item);

    // Put into DynamoDB!
    dynamodb.putItem({
        "TableName": TABLE_NAME,
        "Item": item
    }, function(err, data) {
        if (err) {
            context.fail("ERROR: Dynamo failed: " + err);
        } else {
            console.log("Dynamo Success: " + JSON.stringify(data, null, "  "));
            context.succeed({ success: true });
        }
    });
}