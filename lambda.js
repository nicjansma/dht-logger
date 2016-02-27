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
var TABLE_NAME = "YOURTABLENAME";

var dynamodb = new doc.DynamoDB();

exports.handler = function(event, context) {
    console.log("Request received:\n", JSON.stringify(event));
    console.log("Context received:\n", JSON.stringify(context));

    var datetime = new Date().getTime();

    var item = {
        "device": event.device, 
        "time": datetime,
        "coreid": event.coreid,
        "event": event.event
    };

    // parse data payload
    var data;
    try {
        data = JSON.parse(event.data);
    } catch (e) {
        return context.fail("Failed to parse data JSON");
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
            context.succeed();
        }
    });
}