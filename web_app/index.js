var values_x = [];
var coluomns_colors = []; 
var colors = ["blue", "red", "black"]
var current_color = 0;
var values_y = [];
var ch1;
    

function publish_topic_to_iotcore(msg){
    var myHeaders = new Headers();
    var raw = JSON.stringify({"message":msg});

    var requestOptions = {
        method: 'POST',
        headers: myHeaders,
        body: raw
    };

    fetch("https://w7bp42mbs5.execute-api.eu-west-1.amazonaws.com/dev", requestOptions)
    .then(response => response.text())
    .then()
}

function displayDataOnChart(input){

    input = JSON.parse(input);  // received from API
    //date_test = new Date(parseInt(values[i]["ts"]));

    for(let i=0;i < input.length; i++){
        current_date=new Date();
        hour = current_date.getHours(); // get the hours 
        minute = current_date.getMinutes(); // get the minute
        label = "Time: " + hour.toString()+":" + minute.toString();  // form the label for the coloumn

        coluomns_colors.push(colors[current_color%3]); // assign a color
        current_color++;
        
        ch1.data.labels.push(label);  //add the coloumn
        ch1.data.datasets[0].data[i] = input[i]; // add the values of the coloumn
        ch1.update();
    }

}

function callAPI(){
    var myHeaders = new Headers();
    var requestOptions = {
        method: 'GET',
        headers: myHeaders,
    };
    
    fetch("https://zvuxbnyhci.execute-api.eu-west-1.amazonaws.com/dev", requestOptions)
    .then(response => response.text())
    .then(result => displayDataOnChart(JSON.parse(result).body))
    
}

function createPlot(name, val_y){

    ch = new Chart(name, {
        type: "bar",
        data: {
            labels: values_x,
            datasets: [{
            backgroundColor: coluomns_colors,
            data: val_y
            }]
        },
        options: {
            legend: {display: false},
            title: {
            display: true,
            },
            scales: {
                yAxes: [{
                  ticks: {
                    beginAtZero: true
                  }
                }],
              }

        }
        });
    return ch;


}

function init(){   
    
    ch1=createPlot("Chart1", values_y);
    callAPI();

}
