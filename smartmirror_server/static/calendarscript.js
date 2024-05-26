
///Part for the alarm setter
// Chatgpt code used heavily as base to save time
// first function just creates the dropdown menus and populates them with options, days, hours ... etc.
function populateDropdown(select, start, end, defaultValue) {
    for (let i = start; i <= end; i++) {
        const option = document.createElement('option');
        option.text = i;
        select.add(option);
    }
    if (defaultValue !== undefined) {
        select.value = defaultValue;
    }
}

const yearSelect = document.querySelector('.date-time-picker .year');
const monthSelect = document.querySelector('.date-time-picker .month');
const daySelect = document.querySelector('.date-time-picker .day');
const hourSelect = document.querySelector('.date-time-picker .hour');
const minuteSelect = document.querySelector('.date-time-picker .minute');


const currentYear = new Date().getFullYear();
populateDropdown(yearSelect, currentYear, currentYear + 10, currentYear);
const currentMonth = new Date().getMonth() + 1;
populateDropdown(monthSelect, 1, 12, currentMonth);
const currentDay = new Date().getDate();
populateDropdown(daySelect, 1, 31, currentDay);
const currentHour = new Date().getHours();
populateDropdown(hourSelect, 0, 23, currentHour);
const currentMinute = new Date().getMinutes();
populateDropdown(minuteSelect, 0, 59, currentMinute);


const submitButton = document.querySelector('.date-time-picker .submit-btn');
submitButton.addEventListener('click', function() {
    const selectedYear = yearSelect.value;
    const selectedMonth = monthSelect.value;
    const selectedDay = daySelect.value;
    const selectedHour = hourSelect.value;
    const selectedMinute = minuteSelect.value;

    const selectedDateTime = `${selectedYear},${selectedMonth},${selectedDay},${selectedHour},${selectedMinute}`;
    const selectedDateTime_to_print = `Alarm set  ${selectedDay.padStart(2, '0')}.${selectedMonth.padStart(2, '0')}.${selectedYear} \n ${selectedHour.padStart(2, '0')}:${selectedMinute.padStart(2, '0')}`;

    var showdate_field = document.getElementById("show_date");


    if (selectedYear === "" || selectedMonth === "" || selectedDay === "" || selectedHour === "" || selectedMinute === "") {
    showdate_field.innerHTML = "Set empty alarm" }

    else{
    showdate_field.innerHTML = selectedDateTime_to_print;
    }



    fetch_data(selectedDateTime);
});

// remove all alarms
const removeAllButton = document.querySelector('.remove-all-btn');
removeAllButton.addEventListener('click', function() {
    yearSelect.value = "";
    monthSelect.value = "";
    daySelect.value = "";
    hourSelect.value = "";
    minuteSelect.value = "";
    submitButton.click();
    var showdate_field = document.getElementById("show_date");
    showdate_field.innerHTML = 'Alarms removed';
});




// Post to server
function fetch_data(data_to_send) {
    fetch('/event_submission', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ event: data_to_send }),
        })
        .then(response => {
            if (response.ok) {
                return response.json();
            }
            throw new Error('Network response was not ok.');
        })
        .then(data => {
            console.log(data); // Log the response from the server
            // Handle any additional processing or UI updates here
        })
        .catch(error => {
            console.error('There was a problem with the fetch operation:', error);
            // Handle errors or display error messages to the user
        });
}
