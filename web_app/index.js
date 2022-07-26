var values_x = [];
var coluomns_colors = []; 
var colors = ["blue", "red", "black"]
var current_color = 0;
var values_y = [];
var ch1;
var input = [];
    

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

function displayDataOnChart(data_input){

    data_converted = JSON.parse(data_input);  // received from API
    //date_test = new Date(parseInt(values[i]["ts"]));

    for(let i=0;i < data_converted.length; i++){
        coluomns_colors.push(colors[current_color%3]); // assign a color
        current_color++;
        
        label = data_converted[i]["time"];
        label = "Time:"+label;
        ch1.data.labels.push(label);  //add the coloumn
        ch1.data.datasets[0].data.push(data_converted[i]["value"]); // add the values of the coloumn
        ch1.update();
        input.push([data_converted[i]["time"],data_converted[i]["value"]]);
    }
    document.getElementById("last_usage").innerHTML = input[input.length-1][0];
    document.getElementById("times_usage").innerHTML = input.length;
    total_seconds = 0;
    for(let i=0;i < input.length; i++){
        total_seconds += parseInt(input[i][1]);
    }
    document.getElementById("seconds_usage").innerHTML = total_seconds;

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
