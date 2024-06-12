/**
 * @file app_web_server.h
 * @author Henrique Sander Lourenço
 * @brief Main header file of the app_web_server component.
 * @version 0.1
 * @date 2024-03-16
 *
 * @copyright Copyright (c) 2024 PetDog
 *
 */

#pragma once

#include "esp_err.h"

#define MAIN_PAGE_GET "<!DOCTYPE html><html lang=\"pt-BR\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Comedouro Automático PetDog</title><style>body {background-color: goldenrod;color: midnightblue;padding: 10px;font-family: 'Trebuchet MS', monospace;font-size: 1.5rem;text-align: center;}input {margin-top: 10px;margin-bottom: 10px;}input,button {font-size: 1.2rem;padding: 5px;text-align: center;}footer {margin-top: 30px;}.msg-box {display: none;background-color: brown;color: #eee;padding: 20px;margin: 10px 30px;}</style></head><body><h1>Configure seu Comedouro Automático PetDog!</h1><div class=\"msg-box\" id=\"msgBox\"></div><form action=\"/\" method=\"POST\" id=\"macForm\"><label for=\"mac\">Insira aqui o código de identificação da coleira do seu pet</label><br><input type=\"text\" name=\"mac\" id=\"mac\" placeholder=\"506c931e\"><br><button type=\"button\" id=\"formBtn\">Enviar</button></form><footer>&copy; 2023 Henrique Sander Lourenço</footer><script>form = document.getElementById('macForm');mac = document.getElementById('mac');btn = document.getElementById('formBtn');msgBox = document.getElementById('msgBox');function isValidHexString(input) {const hexPattern = /^[0-9a-fA-F:]+$/;return hexPattern.test(input);}btn.addEventListener('click', (evt) => {errMsg = '';if (mac.value.length != 12) {errMsg = 'MAC com comprimento errado!';} else if (!isValidHexString(mac.value)) {errMsg = 'MAC com caracteres não permitidos!';}if (errMsg.length > 0) {console.log(errMsg);evt.preventDefault();msgBox.textContent = errMsg;msgBox.style.display = 'inline-block';return;}mac.value = mac.value.toLowerCase();form.submit();});</script></body></html>" ///< HTML code for the main page
#define MAIN_PAGE_POST "<!DOCTYPE html><html lang=\"pt-BR\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Comedouro Automático PetDog</title><style>body {background-color: goldenrod;color: midnightblue;padding: 10px;font-family: 'Trebuchet MS', monospace;font-size: 1.5rem;text-align: center;}input,button {font-size: 1.2rem;padding: 5px;}footer {margin-top: 30px;}</style></head><body><h1>Sucesso!</h1><a href=\"/\">Voltar</a><footer>&copy; 2023 Henrique Sander Lourenço</footer></body></html>"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                ///< HTML code for the form submit response page.

esp_err_t app_web_server__start(void);
esp_err_t app_web_server__stop(void);