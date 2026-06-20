// AUTHOR: Emiliano Perez Pellicer
// DATE: 19-06-2026
// COMMENT: This is the Java Script lisetener for the limit of words on the Text Words 
// VERSION: 1.0 


//Keyboard listener 
var textEntered; 

//FromEvent listener 
var elForm;         //Form Element 

//Feedback Elements
var firstNameFeedback;
var lastNameFeedback; 
var emailFeedback; 
var websiteFeedback; 
var birthdayFeedback; 
var prof_or_studentFeedback; 
var involvementFeedback; 

//Get Input Elements
var firstNameInput; 
var lastNameInput; 
var emailInput; 
var websiteInput; 
var birthdayInput; 
var prof_or_studentInput; 
var involvementInput; 

//Gather the elements by ID (INPUT)
elForm = document.getElementById('access');
firstNameInput = document.getElementById('firstname');
lastNameInput = document.getElementById('lastname');
emailInput = document.getElementById('email');
websiteInput = document.getElementById('url');
birthdayInput = document.getElementById('birthday');
prof_or_studentInput = document.getElementById('prof_or_student'); 
involvementInput = document.getElementById('involvement');

//Gather the elements by ID (OUTPUT)
firstNameFeedback = document.getElementById('firstNameFeedback');
lastNameFeedback = document.getElementById('lastNameFeedback');
emailFeedback = document.getElementById('emailFeedback');
websiteFeedback = document.getElementById('websiteFeedback');
birthdayFeedback = document.getElementById('birthdayFeedback');
prof_or_studentFeedback = document.getElementById('prof_or_studentFeedback'); 
involvementFeedback = document.getElementById('involvementFeedback');

//Function listener (Input forms)
function checkInputElements(event){
    //Check for Firstname 
    if (firstNameInput.value.length < 1){
        firstNameFeedback.innerHTML = '<---- You Must enter your First Name'; 
        event.preventDefault();         //Do not Submit the form (submitt == false)
    }else{
        firstNameFeedback.innerHTML = ''; //Clear any error Messages 
    }

    //Check for Lastname 
    if (lastNameInput.value.length < 1){
        lastNameFeedback.innerHTML = '<--- You Must enter your Last Name'; 
        event.preventDefault();            //Do not Submit the Form (FALSE)
    }else{
        lastNameFeedback.innerHTML = '';    //Clear error msg 
    }

    //Check for email
    if(emailInput.value.length < 1){
        emailFeedback.innerHTML = '<--- You Must Enter an Email Address'; 
        event.preventDefault();         //Do not Submit the Form 
    }else{
        emailFeedback.innerHTML = '';     //Clear error msg 
    }

    //Check for website
    if(websiteInput.value.length < 1){
        websiteFeedback.innerHTML = '<--- You Must Enter an URL';
        event.preventDefault();         //DO not submit the form
    }else{
        websiteFeedback.innerHTML = ''; 
    }
    
    //check for birthay 
    if(birthdayInput.value.length < 1){
        birthdayFeedback.innerHTML = '<---- You Must Enter a birthday'; 
        event.preventDefault();         //Do not submit the form 
    }else{
        birthdayFeedback.innerHTML = ''; 
    }

    //check for professor or student
    if(!document.querySelector('input[name="prof_or_student"]:checked')){
        prof_or_studentFeedback.innerHTML = '<--- You Must Select an option'; 
        event.preventDefault();             //Do not submit the form 
    }else{
        prof_or_studentFeedback.innerHTML = ''; //Clear erro msg 
    }

    //check for involvement feedback 
    if(!document.querySelector('input[name="involvement"]:checked')){
        involvementFeedback.innerHTML = '<--- You Must Select an Option'; 
        event.preventDefault(); 
    }else{
        involvementFeedback.innerHTML = ''; 
    }
}

//Function listener (Text area)
function checkTextlen(e){
    //Get the Text area element 
    var textArea = document.getElementById('text_area').value; //Get The Text Area Element (user inptu)
    var elMsg = document.getElementById('feedback'); 
    var charCount = document.getElementById('charactersLeft'); //Characyers remaning 
    var lastChar = document.getElementById('lastkey');        //Get element that displays the last element 

    //Update the counter 
    counter = (180 - (textArea.length)); //Remaming chars 
    charCount.innerHTML = 'Characters remaining:' + counter; //Print remaining chars
    lastChar.innerHTML = 'Last key input:' + String.fromCharCode(e.keyCode); //Output last key 

    //Add the message staemenet 
    if(counter <= 0){
        elMsg.innerHTML = 'You Reached word limit (180)!! '; 
    }else{
        elMsg.innerHTML = ''; //Clear message when back down to limit 
    }
}

//Variable for the keyboard Event 
var textEntered = document.getElementById('text_area'); //Get the message element from Text area 

//Add the Event Listener (Text area)
textEntered.addEventListener('keypress', checkTextlen, false); //Listen for keypress eveent inside the <textarea> element and call my function 

//Add The Event Listener (Form)
elForm.addEventListener('submit', checkInputElements, false); //Listen for the Submit acction 