/***
 * easyhpc menu
 * json file format 
 *  column 0: path; c1: feather icon; c2: url; c3: mime type;
*/
 
const nmFeather = {
    "edit":{"en":"Edit","cn":"修改"},
    "copy":{"en":"Copy","cn":"复制"},
    "trash-2":{"en":"Delete","cn":"删除"}
}

const titleNav = document.querySelector("#title_nav");
const menuId = document.querySelector("#sidebarMenu");
const mainData = document.querySelector("#mainData");
const myModal = new bootstrap.Modal(document.getElementById("myModal"), {backdrop:false});
const myModalTitle = document.getElementById("myModalTitle");
const myModalBody = document.getElementById("myModalBody");


var M = [];
var M0 = [];

const iconFeather = icon => (icon != "")?` data-feather="${icon}" `:" ";

const bulkfun = () => { 
    const chkary = document.getElementsByClassName("form-check-input");
    const bulk = document.getElementById("bulk"); 
    for ( let i = 0; i < chkary.length; i++) {
        if (bulk.checked) chkary[i].checked = true; 
        if (!bulk.checked) chkary[i].checked = false;
    }
} ;

const checkBox = i => `<input class='form-check-input' type='checkbox' name='id' value='${i}' >`;

const perf = (i,a) =>  alert (" this is " + i + " " + a  ) ;


const op = (i, action) => `<span ${ iconFeather(action)} class="icon-color mx-1" data-bs-toggle="tooltip" `
        + ` data-bs-custom-class="custom-tooltip" data-bs-title="${ nmFeather[action]['cn'] }"`
        + `onclick="${action=="trash-2"?"delet":action}(${i})"></span>`;

const viewItem = ( url, id ) => {
        let cont = ""

        fetch ( url + id ) 
        .then( res => res.json() )
        .then ( data => {  
            myModalTitle.textContent = url + id;
            for ( let i = 0 ; i < data.output.length; i++ ) {
                for ( let j = 0; j < data.output[i].length; j++) {
                    cont += data.output[i][j] + " "
                }
                cont += "<br>"    
            }
            myModalBody.innerHTML = cont;                            
            myModal.show();
        }
        )        
}        
const edit = (i) => {
    myModalTitle.textContent = "Edit " + i;
    myModalBody.textContent = "Edit " + i;
    myModal.show();
}
const copy = i => {
    myModalTitle.textContent = "Copy " + i;
    myModalBody.textContent = "Copy " + i;
    myModal.show();
}
const delet = i =>{
    myModalTitle.textContent = "Delete " + i;
    myModalBody.textContent = "Delete " + i;
    myModal.show();
}

const eTable = y => {
    const tblary = y.output
    let cont = "<table class='table table-striped table-bordered'>";

    for ( let i = 0; i < tblary.length; i++ ) {
        if ( i == 0 ) { 
            cont += '<thead><tr>'
            cont += `<th><input id="bulk" type='checkbox' onclick='bulkfun()' ></th>`;
            for ( let j = 0; j < tblary[0].length; j++ ) {
                if ( j == 0 ) { 
                    cont += `<th>${tblary[i][j]}</th>`;
                } else {
                    cont += `<th>${tblary[i][j]}</th>`;
                }
            }
            cont += "<th>操作</th>";
            cont += '</tr></thead><tbody>'
        } else if ( tblary[i].length == tblary[0].length  ) {
            cont +="<tr>";
            cont += "<td>" + checkBox(i) + "</td>";
            for ( let j = 0; j < tblary[i].length; j++ ) {
                if ( j == 0 ) { 
                    cont += `<td><span class="icon-color" onclick="viewItem('${y.read}','${tblary[i][0]}')">${tblary[i][j]}</span></td>`;
                } else {
                    cont += `<td>${tblary[i][j]}</td>`;
                }
            }
            cont += "<td>" + op(i, 'edit' ) + op(i, 'copy' ) + op(i, 'trash-2' ) + "</td>";
            cont += "</tr>";
        }
    }
    cont += "</tbody></table>";
    cont += "<p>" + y + "</p>";
    return cont;  
}
const prnOutput = output => {
    let cont ="<pre>";
    for ( let i = 0; i < output.length; i++ ) {
        for (let j =0 ; j < output[i].length; j++) 
        cont += output[i][j] + "   ";
        cont += "\n"
    }
    return cont += "</pre>";
}

var menuAct = i => {
    let cont ="";
    for ( let n = 0; n < M0[i].length ; n++ ) {
        cont += M0[i][n] ;
        cont += ( n != 0 && n < M0[i].length )?">":"";
    }
    titleNav.innerHTML = cont;
    fetch(M[i][2])
    .then( x => x.json())
    .then ( y => {
        if ( y.template == "eTable") {  
            mainData.innerHTML = eTable( y ); 
            feather.replace(); 
            const tooltipTriggerList = document.querySelectorAll('[data-bs-toggle="tooltip"]')
            const tooltipList = [...tooltipTriggerList].map(tooltipTriggerEl => new bootstrap.Tooltip(tooltipTriggerEl));

        } else if ( y.template == "pre") { 
            mainData.innerHTML = prnOutput (y.output); 
        }
    }); 
}

var menu = (ele,m) => {
        M = m;
        let cont = '<div class="navbar navbar-light navbar-vertical navbar-expand-md">';
        cont += '<ul class="nav flex-column mb-1">';  

        let clev = nlev = 0;
        for ( let i = 0; i < m.length; i++ ) {

            M0[i] = m[i][0].split('/'); 
            
            clev = M0[i].length - 1 ;
            nlev = (i < m.length -1 )?(m[i+1][0].split('/').length -1):0;

            if ( nlev == clev ) {
                cont += `<li class="nav-item">`;
                cont += `<a class="nav-link d-inline-flex" role="button" onclick="menuAct(${i})" >`;
                cont += `<span ` + `${iconFeather(m[i][1])}` + `class="align-baseline"></span>`;
                cont += `<span class="nav-link-text">${M0[i][M0[i].length-1]}</span></a></li>`;
            } else if ( nlev == clev + 1 ) { 
                cont += '<li class="nav-item">'; 
                cont += `<a class="nav-link link-toggle d-inline-flex" role="button" onclick="menuAct(${i})" `;
                cont += `data-bs-target="#${m[i][0]}" `; 
                cont += `data-bs-toggle="collapse" aria-expanded="false" aria-controls="${m[i][0]}">`;
                cont += `<span ` + `${iconFeather(m[i][1])}` ; 
                cont += ' class="align-baseline"></span>';
                cont += `<span class="nav-link-text">${M0[i][M0[i].length-1]}</span></a>`;
                cont += `<ul class="nav flex-column link-toggle-nav collapse ${(i==2)?"ps-2":"ps-3"}" id="${m[i][0]}">`;
            } else if ( nlev < clev ) { 
                cont += `<li class="nav-item">`;
                cont += `<a class="nav-link d-inline-flex" role="button" onclick="menuAct(${i})" >`;
                cont += `<span ` + `${iconFeather(m[i][1])}` + ` class="align-baseline"></span>`;
                cont += `<span class="nav-link-text">${M0[i][M0[i].length-1]}</span></a></li>`;

                let levs = (nlev == 0)?1:nlev;
                for ( let n = clev - levs ; n > 0 ;n--) { 
                        cont += "</ul></li>";
                };
            }
        }
        cont += '</ul></div>';
        ele.innerHTML = cont;
  
        feather.replace();
};

fetch("menu.json")
.then( x => x.json())
.then( y=> menu(menuId,y));
