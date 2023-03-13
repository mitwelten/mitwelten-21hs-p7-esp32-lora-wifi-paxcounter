var node_id = "paxcounter-campus"; // set the node_id here

function Decoder(bytes, port) {
    var decoded = {};
    if (port === 1) {
        decoded.pax = (bytes[0] << 8) + bytes[1];
        decoded.vbatt = (bytes[2] * 2) * 10;
        decoded.macfilter = bytes[3];
        decoded.node_id = node_id;
        decoded.measurement = "pax";
    }
    return decoded;
}