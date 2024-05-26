// Includes all dynamic elements of the website





///  Part for the notes
// Getting the correct element id and adding eventlistener for submit button
document.getElementById("messageForm").addEventListener("submit", function(event) {
    event.preventDefault(); // prevents immediate submission of the form and reload of the page

    var inputData = document.getElementById("messageInput").value; // getting the data from the field

    // sending data to the server, post method is also specified in the server-side
    fetch("/note-receive", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify({ data: inputData })
    })
    .then(response => {
        if (!response.ok) {
            throw new Error("Network response was not ok");
        }
        return response.json(); // in case of success run function to get json form of the data, needed for the next step
    })
    .then(data => {
        console.log("Server response:", data); // maybe needed for debugging or the note could also be shown in the site with this


    })
    .catch(error => {
        //console.error("There was a problem with your fetch operation:", error); // Debugging
    });
});



/// Part for the theme change
//function for setting the theme visible
function setTheme(themeName) {
    localStorage.setItem('theme', themeName); // localstorage can be used to save information for current session, in this case we want to remember the theme on current session
    document.documentElement.className = themeName; // accesses class from html and sets it to themeName, first line of index2.html is <html lang="en"  class="theme-light">, this actually makes the new theme visible
}
// function for toggling the new theme
function toggleTheme() {
   if (localStorage.getItem('theme') === 'theme-dark'){ // gets the theme from local storage
       setTheme('theme-light');  // uses previous function to set it
   } else {
       setTheme('theme-dark');
   }
}
// special js function IIFE (immediately invoked function) which runs immediately when the js is run right after its defined and doesnt wait for function call
(function () {
   if (localStorage.getItem('theme') === 'theme-dark') {
       setTheme('theme-dark');
   } else {
       setTheme('theme-light');
   }
})();


//// Bus part
// second part of the bus was to submit the stop and then create list to view
document.getElementById("submitButton").addEventListener("click", function() { // acessing the submit button and adding event listener click on it
    var selectedValue = document.getElementById("responseDropdown").value;  // accessing the value chosen from the dropdown
    // sending the value to the server and it invokes a function there altering the database aswell
    fetch("/send-selected-value", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify({ data: selectedValue })
    })
    .then(response => {
        if (!response.ok) {
            throw new Error("Network response was not ok");
        }
        return response.json();
    })
    .then(data => {
        console.log("Server response after sending selected value:", data); // again for debugging and showing the values on the website

        var serverResponseDiv = document.getElementById("serverResponseDepartures"); // special place in the html code for the values to be shown on the screen
        serverResponseDiv.innerHTML = ""; // emptying it first
        var stopTimesList = document.createElement("ul"); // creating undordered list html element

        // {18540: '71 Rautatientori via Sörnäinen(M)'}  // looping through the data and adding it to the html list element
        Object.keys(data).forEach(stop => {
            var stopTimeItem = document.createElement("li");
            stopTimeItem.textContent = `${stop}: ${data[stop][0]}`;
            stopTimesList.appendChild(stopTimeItem);
        });
        serverResponseDiv.appendChild(stopTimesList); // this actually appends the new list to the ul element

    })
    .catch(error => {
        console.error("There was a problem with your fetch operation:", error);
    });
});



// first part was to type your stop/area and then get the list of stops in that area
document.getElementById("dataForm").addEventListener("submit", function(event) {
    event.preventDefault();

    var inputData = document.getElementById("dataInput").value; // get the input value like 'Kumpulan kamp'
    // sends it to the server where server does API get to digitransit
    fetch("/transport-receive", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify({ data: inputData })
    })
    .then(response => {
        if (!response.ok) {
            throw new Error("Network response was not ok");
        }
        return response.json();
    })
    .then(data => {
        //console.log("Server response:", data); //debugging
        var dropdown = document.getElementById("responseDropdown"); // access dropdown menu
        dropdown.innerHTML = ""; // empty first
        // adding all the options received from the server (which uses api get to digitransit to query stops in that area)
        data.choices.forEach(choice => {
            var option = document.createElement("option");
            option.value = choice.value;
            option.textContent = choice.label;
            dropdown.appendChild(option);
        });
    })
    .catch(error => {
        //console.error("There was a problem with your fetch operation:", error);

    });
});




// Updating the dht value
// When Dht value is 0 dont draw anything meaning the old value will be shown
function updateDHT(new_DHT_values){
    const DHTDataSection = document.getElementById('DHT-data-section');
    temperature=new_DHT_values['value1'];
    humidity=new_DHT_values['value2'];
    if (temperature !== '0.00' && humidity !== '0.00') { // fix for 0 values which we dont want to see

        DHTDataSection.innerHTML = `
          <h3>DHT Data</h3>
          <p>Temperature: ${new_DHT_values["value1"]}</p>
          <p>Humidity: ${new_DHT_values["value2"]}</p>`;
    }
    else{

    }

}

// function for fetching data from the main data route
// initially most of the data would be update and show but now it's just DHT data, would be very easy to implement, just rendering
// Since this launches api requests to digitransit etc. would be better to handle this in other way
function fetchData() {
    fetch('/main_data?Kumpula')
      .then(response => {
        if (!response.ok) {
          throw new Error('Network response was not ok');
        }
        return response.json();
      })
      .then(data => {

        const DHT_data=data.dht_data;

        updateDHT(DHT_data);


      })
      .catch(error => {
        console.error('There was a problem with the fetch operation:', error);
      });
  }

//first data fetch
fetchData();
// fetch every 5 sec
const intervalSeconds = 5;
setInterval(fetchData, intervalSeconds * 1000); // Convert seconds to milliseconds


