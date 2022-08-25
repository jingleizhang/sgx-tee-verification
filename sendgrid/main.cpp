#include <iostream>
#include "sendgridmsg.h"

int main(int argc, char *argv[])
{
    SendGrid::SendGridMessage msg;
    msg.setSubject("SendGrid Test");
    msg.setFrom(new SendGrid::EmailAddress{"from@example.com", "from"});
    msg.setTemplateId("setTemplate Id");

    SendGrid::Personalization p;
    p.to.push_back(SendGrid::EmailAddress{"to@example.com"});
    neb::CJsonObject templateData;
    templateData.Add("verification_link", "http://sample.verification.link");
    p.SetDynamicTemplateData(templateData);
    msg.addPersonalization(p);

    std::cout << msg.toString() << std::endl;
}