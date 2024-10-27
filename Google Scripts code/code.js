function doGet(e) {
  try {
    var action = e.parameter.action;
    Logger.log("Action: " + action);

    if (action == "getList") {
      return getList();
    } else if (action == "markAttendance") {
      return markAttendance(e);
    } else if (action == "register") {
      return registerStudent(e);
    } else if (action == "remove") {
      return removeStudent(e);
    } else if (action == "resetAttendance") {
      return resetAttendance();
    } else {
      return ContentService.createTextOutput("Invalid action.");
    }
  } catch (error) {
    Logger.log("Error in doGet: " + error);
    return ContentService.createTextOutput("An error occurred: " + error);
  }
}

function getList() {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheetByName("Sheet1"); // Replace with your sheet name
  var data = sheet.getDataRange().getValues();

  Logger.log("Data fetched from sheet: " + JSON.stringify(data));

  var students = [];
  var totalStudents = data.length - 1; // Exclude header row
  var studentsOnBus = 0;

  for (var i = 1; i < data.length; i++) {
    var row = data[i];
    var student = {
      rfid: row[0] || "",
      name: row[1] || "",
      admission: row[2] || "",
      onBus: row[3] || false,
      time: row[4] ? Utilities.formatDate(new Date(row[4]), Session.getScriptTimeZone(), "yyyy-MM-dd HH:mm:ss") : ""
    };
    if (row[3] == true) {
      studentsOnBus++;
    }
    students.push(student);
  }

  var result = {
    totalStudents: totalStudents,
    studentsOnBus: studentsOnBus,
    students: students
  };

  Logger.log("Resulting JSON: " + JSON.stringify(result));

  var jsonOutput = ContentService.createTextOutput(JSON.stringify(result));
  jsonOutput.setMimeType(ContentService.MimeType.JSON);
  return jsonOutput;
}

function markAttendance(e) {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheetByName("Sheet1");
  var data = sheet.getDataRange().getValues();
  var rfid = e.parameter.rfid;
  var found = false;

  for (var i = 1; i < data.length; i++) {
    if (data[i][0] == rfid) {
      found = true;
      var onBus = data[i][3];
      // Toggle the OnBus status
      sheet.getRange(i + 1, 4).setValue(!onBus);
      // Update the Time
      var timestamp = new Date();
      sheet.getRange(i + 1, 5).setValue(timestamp);
      break;
    }
  }

  if (!found) {
    return ContentService.createTextOutput("RFID not found.");
  } else {
    return ContentService.createTextOutput("Attendance updated.");
  }
}

function registerStudent(e) {
  try {
    var ss = SpreadsheetApp.getActiveSpreadsheet();
    var sheet = ss.getSheetByName("Sheet1");
    var rfid = e.parameter.rfid;
    var name = e.parameter.name;
    var admission = e.parameter.admission;

    Logger.log("Registering student: RFID=" + rfid + ", Name=" + name + ", Admission=" + admission);

    if (!rfid || !name || !admission) {
      return ContentService.createTextOutput("Missing parameters.");
    }

    // Check if RFID already exists
    var data = sheet.getDataRange().getValues();
    for (var i = 1; i < data.length; i++) {
      if (data[i][0] == rfid) {
        return ContentService.createTextOutput("RFID already registered.");
      }
    }

    // Append the new student
    sheet.appendRow([rfid, name, admission, false, ""]);

    return ContentService.createTextOutput("Student registered.");
  } catch (error) {
    Logger.log("Error in registerStudent: " + error);
    return ContentService.createTextOutput("An error occurred in registerStudent.");
  }
}

function removeStudent(e) {
  try {
    var ss = SpreadsheetApp.getActiveSpreadsheet();
    var sheet = ss.getSheetByName("Sheet1");
    var rfid = e.parameter.rfid;
    var data = sheet.getDataRange().getValues();
    var found = false;

    for (var i = data.length - 1; i >= 1; i--) {
      if (data[i][0] == rfid) {
        sheet.deleteRow(i + 1);
        found = true;
        break;
      }
    }

    if (found) {
      return ContentService.createTextOutput("Student removed.");
    } else {
      return ContentService.createTextOutput("RFID not found.");
    }
  } catch (error) {
    Logger.log("Error in removeStudent: " + error);
    return ContentService.createTextOutput("An error occurred in removeStudent.");
  }
}

function resetAttendance() {
  try {
    var ss = SpreadsheetApp.getActiveSpreadsheet();
    var sheet = ss.getSheetByName("Sheet1");
    var data = sheet.getDataRange().getValues();

    for (var i = 1; i < data.length; i++) {
      // Set OnBus status to false
      sheet.getRange(i + 1, 4).setValue(false);
      // Clear the Time column
      sheet.getRange(i + 1, 5).clearContent();
    }

    return ContentService.createTextOutput("Attendance reset.");
  } catch (error) {
    Logger.log("Error in resetAttendance: " + error);
    return ContentService.createTextOutput("An error occurred in resetAttendance.");
  }
}
