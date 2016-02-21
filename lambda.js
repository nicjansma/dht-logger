//
// Creates a DynamoDB row with your IoT data.
// By Nic Jansma http://nicj.net
//
// https://github.com/nicjansma/dht-logger/
//
// Based on: https://snowulf.com/2015/08/05/tutorial-aws-api-gateway-to-lambda-to-dynamodb/
//
console.log("Loading event");

var doc = require("dynamodb-doc");
var dynamodb = new doc.DynamoDB();

exports.handler = function(event, context) {
    console.log("Request received:\n", JSON.stringify(event));
    console.log("Context received:\n", JSON.stringify(context));

    var tableName = "YOURTABLENAME";

    var datetime = new Date().getTime();

    var item = {
        "device": event.device, 
        "time": datetime,
    };

    for (var key in event) {
        // skip some Particle Webhook properties
        if (key === "data" || key === "published_at") {
            continue;
        }

        if (event.hasOwnProperty(key)) {
            item[key] = event[key];
        }
    }

    console.log("Item:\n", item);

    dynamodb.putItem({
        "TableName": tableName,
        "Item": item
    }, function(err, data) {
        if (err) {
            context.fail("ERROR: Dynamo failed: " + err);
        } else {
            console.log("Dynamo Success: " + JSON.stringify(data, null, "  "));
            context.succeed("SUCCESS");
        }
    });
}